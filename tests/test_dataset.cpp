// Tests for Dataset.
//
//   test_dataset                      -> runs the unit tests on tests/sample.csv
//   test_dataset <path/to/big.csv>    -> also loads the real dataset and prints stats
//
// std::sort is used here ONLY to check that Dataset::less() is a valid ordering.
// The project's own quicksort/mergesort live elsewhere.

#include "../src/Dataset.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

static int failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            ++failures;                                                          \
            std::printf("  FAIL line %d: %s\n", __LINE__, #cond);                \
        }                                                                        \
    } while (0)

static std::vector<std::string> sortedColumn(const Dataset& d, const std::string& name) {
    int col = d.columnIndex(name);
    std::vector<int> order = d.identityOrder();
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return d.less(col, a, b); });
    std::vector<std::string> out;
    for (int r : order) out.push_back(d.cell(col, r));
    return out;
}

static void unitTests(const std::string& samplePath) {
    std::printf("Unit tests on %s\n", samplePath.c_str());
    Dataset d;
    d.load(samplePath);

    CHECK(d.numRows() == 4);           // trailing blank line ignored
    CHECK(d.numColumns() == 11);
    CHECK(d.columnIndex("price") == 2);
    CHECK(d.columnIndex("nope") == -1);

    // type inference
    CHECK(d.columnType(d.columnIndex("id")) == ColumnType::Text);
    CHECK(d.columnType(d.columnIndex("price")) == ColumnType::Real);
    CHECK(d.columnType(d.columnIndex("qty")) == ColumnType::Integer);
    CHECK(d.columnType(d.columnIndex("day")) == ColumnType::Date);
    CHECK(d.columnType(d.columnIndex("at")) == ColumnType::Time);
    CHECK(d.columnType(d.columnIndex("stamp")) == ColumnType::DateTime);
    CHECK(d.columnType(d.columnIndex("code")) == ColumnType::Text);     // leading zeros stay text
    CHECK(d.columnType(d.columnIndex("mix")) == ColumnType::Real);      // 1, 2.5, 3, 4
    CHECK(d.columnType(d.columnIndex("lvl")) == ColumnType::Text);      // 1, 2, N/A, 3
    CHECK(d.columnType(d.columnIndex("blank_col")) == ColumnType::Text);

    // cell values round-trip
    int name = d.columnIndex("name"), price = d.columnIndex("price");
    CHECK(d.cell(name, 0) == "Smith, Jo");       // quoted comma
    CHECK(d.cell(name, 2) == "Say \"hi\"");      // escaped quotes
    CHECK(d.cell(price, 0) == "10.5");
    CHECK(d.cell(price, 3) == "100.6");
    CHECK(d.cell(d.columnIndex("qty"), 1) == "-4");
    CHECK(d.cell(d.columnIndex("day"), 0) == "2023-07-01");
    CHECK(d.cell(d.columnIndex("at"), 1) == "23:59:59");
    CHECK(d.cell(d.columnIndex("stamp"), 2) == "2024-01-15 00:00:00");
    CHECK(d.cell(d.columnIndex("code"), 0) == "0012");
    CHECK(d.number(price, 1) == 9.2);

    // blanks
    CHECK(d.isMissing(price, 2));
    CHECK(!d.isMissing(price, 0));
    CHECK(d.isMissing(name, 3));
    CHECK(d.column(price).missing == 1);
    CHECK(d.column(d.columnIndex("blank_col")).missing == 4);

    // ordering: numbers numerically, blanks last
    CHECK((sortedColumn(d, "price") == std::vector<std::string>{"9.2", "10.5", "100.6", ""}));
    CHECK((sortedColumn(d, "qty") == std::vector<std::string>{"-4", "0", "3", "10"}));
    CHECK((sortedColumn(d, "day") == std::vector<std::string>{"2022-12-31", "2023-07-01", "2024-01-15", ""}));
    CHECK((sortedColumn(d, "at") == std::vector<std::string>{"00:00:00", "06:20:09", "12:00:00", "23:59:59"}));
    CHECK((sortedColumn(d, "name") == std::vector<std::string>{"Apple", "Say \"hi\"", "Smith, Jo", ""}));
    CHECK(d.compare(price, 0, 0) == 0);
    CHECK(d.compare(price, 1, 0) < 0 && d.compare(price, 0, 1) > 0);

    // loading only some columns
    Dataset small;
    small.load(samplePath, {"qty", "id"});
    CHECK(small.numColumns() == 2);
    CHECK(small.columnName(0) == "qty" && small.columnName(1) == "id");
    CHECK(small.cell(1, 3) == "T4");

    // errors
    bool threw = false;
    try { Dataset x; x.load("does/not/exist.csv"); } catch (const std::runtime_error&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { Dataset x; x.load(samplePath, {"not_a_column"}); } catch (const std::runtime_error&) { threw = true; }
    CHECK(threw);
}

static void bigFile(const std::string& path) {
    std::printf("\nLoading %s ...\n", path.c_str());
    Dataset d;
    d.load(path);
    std::printf("  rows:     %zu\n", d.numRows());
    std::printf("  columns:  %zu\n", d.numColumns());
    std::printf("  load:     %.2f s\n", d.loadSeconds());
    std::printf("  memory:   %.1f MB\n", d.approxBytes() / (1024.0 * 1024.0));

    std::map<std::string, int> types;
    for (std::size_t c = 0; c < d.numColumns(); ++c) types[columnTypeName(d.columnType(static_cast<int>(c)))]++;
    std::printf("  types:   ");
    for (auto& t : types) std::printf(" %s=%d", t.first.c_str(), t.second);
    std::printf("\n");

    const char* show[] = {"transaction_id", "transaction_date", "transaction_datetime", "product_category",
                          "quantity", "total_amount", "customer_engagement_score"};
    for (const char* name : show) {
        int col = d.columnIndex(name);
        CHECK(col >= 0);
        if (col < 0) continue;
        std::printf("  %-26s %-9s blanks=%-5zu row0=%s\n", name, columnTypeName(d.columnType(col)),
                    d.column(col).missing, d.cell(col, 0).c_str());
    }

    // The comparator must give a consistent order on real data.
    for (const char* name : {"total_amount", "transaction_datetime", "product_category", "customer_engagement_score"}) {
        int col = d.columnIndex(name);
        std::vector<int> order = d.identityOrder();
        std::sort(order.begin(), order.end(), [&](int a, int b) { return d.less(col, a, b); });
        bool ok = true;
        for (std::size_t i = 1; i < order.size(); ++i)
            if (d.less(col, order[i], order[i - 1])) { ok = false; break; }
        CHECK(ok);
        std::printf("  sorted by %-26s first=%-20s last=%s\n", name, d.cell(col, order.front()).c_str(),
                    d.cell(col, order.back()).c_str());
    }
}

int main(int argc, char** argv) {
    try {
        unitTests("tests/sample.csv");
        if (argc > 1) bigFile(argv[1]);
    } catch (const std::exception& e) {
        std::printf("EXCEPTION: %s\n", e.what());
        return 1;
    }
    std::printf(failures == 0 ? "\nALL TESTS PASSED\n" : "\n%d CHECK(S) FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
