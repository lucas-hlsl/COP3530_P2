#include "Dataset.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <limits>
#include <stdexcept>

// ============================================================================
// Small helpers for reading the CSV text
// ============================================================================
namespace {

// A field is a view into the file buffer (no copying).
struct Field {
    const char* ptr;
    std::size_t len;
};

// Splits one line [begin, end) on commas. Supports "quoted, fields" and ""
// escapes; quoted fields that contain "" are unescaped into `owned` so the
// views stay valid until the next call. (Newlines inside quotes are not
// supported; this dataset has no quotes at all.)
void splitLine(const char* begin, const char* end,
               std::vector<Field>& out, std::deque<std::string>& owned) {
    out.clear();
    owned.clear();
    const char* p = begin;
    while (true) {
        if (p < end && *p == '"') {
            ++p;
            const char* start = p;
            bool hasEscapes = false;
            while (p < end) {
                if (*p == '"') {
                    if (p + 1 < end && p[1] == '"') { hasEscapes = true; p += 2; continue; }
                    break;
                }
                ++p;
            }
            if (!hasEscapes) {
                out.push_back({start, static_cast<std::size_t>(p - start)});
            } else {
                std::string s;
                for (const char* q = start; q < p; ++q) {
                    s.push_back(*q);
                    if (*q == '"') ++q;   // skip the second quote of ""
                }
                owned.push_back(std::move(s));
                out.push_back({owned.back().data(), owned.back().size()});
            }
            if (p < end) ++p;                       // closing quote
            while (p < end && *p != ',') ++p;       // ignore junk after the quote
        } else {
            const char* start = p;
            while (p < end && *p != ',') ++p;
            out.push_back({start, static_cast<std::size_t>(p - start)});
        }
        if (p >= end) break;
        ++p;                                        // skip the comma
    }
}

bool isDigit(char c) { return c >= '0' && c <= '9'; }

int digits(const char* p, int n) {
    int v = 0;
    for (int i = 0; i < n; ++i) v = v * 10 + (p[i] - '0');
    return v;
}

// What one cell looks like. Columns get the "widest" kind seen in any row.
enum class Kind { Empty, Integer, Real, Date, Time, DateTime, Text };

bool looksLikeNumber(const Field& f, bool& isInteger) {
    const char* p = f.ptr;
    const char* e = f.ptr + f.len;
    if (p < e && (*p == '-' || *p == '+')) ++p;
    const char* intStart = p;
    while (p < e && isDigit(*p)) ++p;
    std::size_t intDigits = static_cast<std::size_t>(p - intStart);
    // A leading zero ("0123") means it's a code, not a number - keep it as text.
    if (intDigits > 1 && *intStart == '0') return false;
    std::size_t fracDigits = 0;
    isInteger = true;
    if (p < e && *p == '.') {
        isInteger = false;
        ++p;
        const char* fracStart = p;
        while (p < e && isDigit(*p)) ++p;
        fracDigits = static_cast<std::size_t>(p - fracStart);
    }
    if (intDigits + fracDigits == 0) return false;
    if (p < e && (*p == 'e' || *p == 'E')) {
        isInteger = false;
        ++p;
        if (p < e && (*p == '-' || *p == '+')) ++p;
        const char* expStart = p;
        while (p < e && isDigit(*p)) ++p;
        if (p == expStart) return false;
    }
    return p == e;
}

Kind classify(const Field& f) {
    if (f.len == 0) return Kind::Empty;
    const char* s = f.ptr;
    if (f.len == 10 && s[4] == '-' && s[7] == '-' &&
        isDigit(s[0]) && isDigit(s[1]) && isDigit(s[2]) && isDigit(s[3]) &&
        isDigit(s[5]) && isDigit(s[6]) && isDigit(s[8]) && isDigit(s[9]))
        return Kind::Date;
    if (f.len == 8 && s[2] == ':' && s[5] == ':' &&
        isDigit(s[0]) && isDigit(s[1]) && isDigit(s[3]) && isDigit(s[4]) &&
        isDigit(s[6]) && isDigit(s[7]))
        return Kind::Time;
    if (f.len == 19 && s[10] == ' ' &&
        classify({s, 10}) == Kind::Date && classify({s + 11, 8}) == Kind::Time)
        return Kind::DateTime;
    bool isInteger = false;
    if (looksLikeNumber(f, isInteger)) return isInteger ? Kind::Integer : Kind::Real;
    return Kind::Text;
}

Kind merge(Kind current, Kind seen) {
    if (seen == Kind::Empty || seen == current) return current;
    if (current == Kind::Empty) return seen;
    bool bothNumeric = (current == Kind::Integer || current == Kind::Real) &&
                       (seen == Kind::Integer || seen == Kind::Real);
    return bothNumeric ? Kind::Real : Kind::Text;
}

ColumnType toColumnType(Kind k) {
    switch (k) {
        case Kind::Integer:  return ColumnType::Integer;
        case Kind::Real:     return ColumnType::Real;
        case Kind::Date:     return ColumnType::Date;
        case Kind::Time:     return ColumnType::Time;
        case Kind::DateTime: return ColumnType::DateTime;
        default:             return ColumnType::Text;   // Text, or a column that is all blank
    }
}

// Fast, exact parse for plain decimals like "105.63" (<= 15 significant digits,
// no exponent): mantissa / 10^k is correctly rounded, so the result equals
// strtod's. Anything else falls back to strtod.
double parseNumber(const Field& f) {
    static const double kPow10[] = {1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8,
                                    1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16,
                                    1e17, 1e18, 1e19, 1e20, 1e21, 1e22};
    const char* p = f.ptr;
    const char* e = f.ptr + f.len;
    bool negative = false;
    if (p < e && (*p == '-' || *p == '+')) { negative = (*p == '-'); ++p; }
    std::uint64_t mantissa = 0;
    int significant = 0, fraction = 0;
    bool inFraction = false, simple = true;
    for (; p < e; ++p) {
        if (isDigit(*p)) {
            if (mantissa != 0 || *p != '0') ++significant;
            mantissa = mantissa * 10 + static_cast<std::uint64_t>(*p - '0');
            if (inFraction) ++fraction;
            if (significant > 15) { simple = false; break; }
        } else if (*p == '.' && !inFraction) {
            inFraction = true;
        } else {
            simple = false;   // exponent or something unexpected
            break;
        }
    }
    if (simple && fraction <= 22) {
        double v = static_cast<double>(mantissa) / kPow10[fraction];
        return negative ? -v : v;
    }
    std::string copy(f.ptr, f.len);   // strtod needs a terminated string
    return std::strtod(copy.c_str(), nullptr);
}

double parseValue(ColumnType type, const Field& f) {
    const char* s = f.ptr;
    switch (type) {
        case ColumnType::Date:
            return digits(s, 4) * 10000.0 + digits(s + 5, 2) * 100.0 + digits(s + 8, 2);
        case ColumnType::Time:
            return digits(s, 2) * 3600.0 + digits(s + 3, 2) * 60.0 + digits(s + 6, 2);
        case ColumnType::DateTime:
            return (digits(s, 4) * 10000.0 + digits(s + 5, 2) * 100.0 + digits(s + 8, 2)) * 1000000.0 +
                   digits(s + 11, 2) * 10000.0 + digits(s + 14, 2) * 100.0 + digits(s + 17, 2);
        default:
            return parseNumber(f);
    }
}

// Iterates over the lines of the buffer, skipping blank ones and stripping '\r'.
struct LineReader {
    const char* p;
    const char* end;
    std::size_t lineNumber = 0;   // 1-based, for error messages

    bool next(const char*& lineBegin, const char*& lineEnd) {
        while (p < end) {
            const char* b = p;
            while (p < end && *p != '\n') ++p;
            const char* e = p;
            if (p < end) ++p;   // skip '\n'
            ++lineNumber;
            if (e > b && e[-1] == '\r') --e;
            if (e == b) continue;   // blank line
            lineBegin = b;
            lineEnd = e;
            return true;
        }
        return false;
    }
};

}  // namespace

// ============================================================================
// Dataset
// ============================================================================

const char* columnTypeName(ColumnType type) {
    switch (type) {
        case ColumnType::Integer:  return "Integer";
        case ColumnType::Real:     return "Real";
        case ColumnType::Date:     return "Date";
        case ColumnType::Time:     return "Time";
        case ColumnType::DateTime: return "DateTime";
        case ColumnType::Text:     return "Text";
    }
    return "?";
}

void Dataset::load(const std::string& path, const std::vector<std::string>& onlyColumns) {
    auto startTime = std::chrono::steady_clock::now();
    columns_.clear();
    index_.clear();
    rows_ = 0;

    // ---- read the whole file into memory (one big read is much faster than getline) ----
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Dataset: could not open '" + path + "'");
    file.seekg(0, std::ios::end);
    std::streamoff size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string buffer(static_cast<std::size_t>(size), '\0');
    if (size > 0 && !file.read(&buffer[0], size))
        throw std::runtime_error("Dataset: could not read '" + path + "'");
    file.close();

    const char* begin = buffer.data();
    const char* end = begin + buffer.size();
    if (buffer.size() >= 3 && static_cast<unsigned char>(begin[0]) == 0xEF &&
        static_cast<unsigned char>(begin[1]) == 0xBB && static_cast<unsigned char>(begin[2]) == 0xBF)
        begin += 3;   // UTF-8 byte-order mark

    std::vector<Field> fields;
    std::deque<std::string> owned;
    const char* lineBegin;
    const char* lineEnd;

    // ---- header ----
    LineReader header{begin, end};
    if (!header.next(lineBegin, lineEnd)) throw std::runtime_error("Dataset: '" + path + "' is empty");
    splitLine(lineBegin, lineEnd, fields, owned);
    std::vector<std::string> headerNames;
    for (const Field& f : fields) headerNames.emplace_back(f.ptr, f.len);
    const std::size_t fileColumns = headerNames.size();
    const char* dataStart = header.p;
    const std::size_t headerLine = header.lineNumber;

    // Which file columns to keep, and where each one goes in columns_.
    std::vector<int> keep(fileColumns, -1);
    if (onlyColumns.empty()) {
        for (std::size_t i = 0; i < fileColumns; ++i) keep[i] = static_cast<int>(i);
    } else {
        int next = 0;
        for (const std::string& wanted : onlyColumns) {
            bool found = false;
            for (std::size_t i = 0; i < fileColumns; ++i) {
                if (headerNames[i] == wanted && keep[i] == -1) { keep[i] = next++; found = true; break; }
            }
            if (!found) throw std::runtime_error("Dataset: column '" + wanted + "' is not in the header");
        }
    }
    std::size_t kept = 0;
    for (int k : keep) if (k >= 0) ++kept;
    columns_.resize(kept);
    for (std::size_t i = 0; i < fileColumns; ++i)
        if (keep[i] >= 0) columns_[keep[i]].name = headerNames[i];

    // ---- pass 1: count rows and work out each column's type ----
    std::vector<Kind> kinds(fileColumns, Kind::Empty);
    LineReader pass1{dataStart, end, headerLine};
    while (pass1.next(lineBegin, lineEnd)) {
        splitLine(lineBegin, lineEnd, fields, owned);
        if (fields.size() != fileColumns)
            throw std::runtime_error("Dataset: line " + std::to_string(pass1.lineNumber) + " has " +
                                     std::to_string(fields.size()) + " fields but the header has " +
                                     std::to_string(fileColumns));
        for (std::size_t i = 0; i < fileColumns; ++i)
            if (keep[i] >= 0 && kinds[i] != Kind::Text) kinds[i] = merge(kinds[i], classify(fields[i]));
        ++rows_;
    }

    // Size every column exactly once (no growth while filling).
    std::vector<std::unordered_map<std::string, std::uint32_t>> lookup(kept);
    for (std::size_t i = 0; i < fileColumns; ++i) {
        if (keep[i] < 0) continue;
        Column& c = columns_[keep[i]];
        c.type = toColumnType(kinds[i]);
        if (c.isText()) c.codes.resize(rows_);
        else c.numbers.resize(rows_);
    }

    // ---- pass 2: fill the columns ----
    const double nan = std::numeric_limits<double>::quiet_NaN();
    LineReader pass2{dataStart, end, headerLine};
    std::size_t row = 0;
    std::string key;
    while (pass2.next(lineBegin, lineEnd)) {
        splitLine(lineBegin, lineEnd, fields, owned);
        for (std::size_t i = 0; i < fileColumns; ++i) {
            int out = keep[i];
            if (out < 0) continue;
            Column& c = columns_[out];
            const Field& f = fields[i];
            if (f.len == 0) ++c.missing;
            if (c.isText()) {
                key.assign(f.ptr, f.len);
                auto it = lookup[out].find(key);
                std::uint32_t code;
                if (it != lookup[out].end()) {
                    code = it->second;
                } else {
                    code = static_cast<std::uint32_t>(c.dictionary.size());
                    c.dictionary.push_back(key);
                    lookup[out].emplace(key, code);
                }
                c.codes[row] = code;
            } else {
                c.numbers[row] = (f.len == 0) ? nan : parseValue(c.type, f);
            }
        }
        ++row;
    }

    for (Column& c : columns_) {
        c.dictionary.shrink_to_fit();
        for (std::string& s : c.dictionary) s.shrink_to_fit();
    }
    for (std::size_t i = 0; i < columns_.size(); ++i)
        index_.emplace(columns_[i].name, static_cast<int>(i));

    loadSeconds_ = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
}

int Dataset::columnIndex(const std::string& name) const {
    auto it = index_.find(name);
    return it == index_.end() ? -1 : it->second;
}

std::vector<std::string> Dataset::columnNames() const {
    std::vector<std::string> names;
    names.reserve(columns_.size());
    for (const Column& c : columns_) names.push_back(c.name);
    return names;
}

bool Dataset::isMissing(int col, int row) const {
    const Column& c = columns_[col];
    if (c.isText()) return c.dictionary[c.codes[row]].empty();
    return std::isnan(c.numbers[row]);
}

double Dataset::number(int col, int row) const {
    return columns_[col].numbers[row];
}

const std::string& Dataset::text(int col, int row) const {
    const Column& c = columns_[col];
    return c.dictionary[c.codes[row]];
}

std::string Dataset::cell(int col, int row) const {
    const Column& c = columns_[col];
    if (c.isText()) return c.dictionary[c.codes[row]];
    double v = c.numbers[row];
    if (std::isnan(v)) return "";
    char buf[32];
    long long n = static_cast<long long>(v);
    switch (c.type) {
        case ColumnType::Integer:
            std::snprintf(buf, sizeof buf, "%.0f", v);
            break;
        case ColumnType::Date:
            std::snprintf(buf, sizeof buf, "%04lld-%02lld-%02lld", n / 10000, n / 100 % 100, n % 100);
            break;
        case ColumnType::Time:
            std::snprintf(buf, sizeof buf, "%02lld:%02lld:%02lld", n / 3600, n / 60 % 60, n % 60);
            break;
        case ColumnType::DateTime: {
            long long d = n / 1000000, t = n % 1000000;
            std::snprintf(buf, sizeof buf, "%04lld-%02lld-%02lld %02lld:%02lld:%02lld",
                          d / 10000, d / 100 % 100, d % 100, t / 10000, t / 100 % 100, t % 100);
            break;
        }
        default:
            std::snprintf(buf, sizeof buf, "%.15g", v);
            break;
    }
    return buf;
}

int Dataset::compare(int col, int rowA, int rowB) const {
    const Column& c = columns_[col];
    if (c.isText()) {
        std::uint32_t a = c.codes[rowA], b = c.codes[rowB];
        if (a == b) return 0;
        const std::string& sa = c.dictionary[a];
        const std::string& sb = c.dictionary[b];
        if (sa.empty()) return 1;    // blanks last
        if (sb.empty()) return -1;
        return sa.compare(sb);
    }
    double a = c.numbers[rowA], b = c.numbers[rowB];
    bool aMissing = std::isnan(a), bMissing = std::isnan(b);
    if (aMissing || bMissing) return static_cast<int>(aMissing) - static_cast<int>(bMissing);
    return (a < b) ? -1 : (a > b) ? 1 : 0;
}

std::vector<int> Dataset::identityOrder() const {
    std::vector<int> order(rows_);
    for (std::size_t i = 0; i < rows_; ++i) order[i] = static_cast<int>(i);
    return order;
}

std::size_t Dataset::approxBytes() const {
    std::size_t total = 0;
    for (const Column& c : columns_) {
        total += sizeof(Column) + c.name.capacity();
        total += c.numbers.capacity() * sizeof(double);
        total += c.codes.capacity() * sizeof(std::uint32_t);
        for (const std::string& s : c.dictionary)
            total += sizeof(std::string) + (s.capacity() > 15 ? s.capacity() + 1 : 0);
    }
    return total;
}
