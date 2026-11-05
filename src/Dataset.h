#pragma once
// Dataset: an in-memory, column-oriented, typed copy of a CSV file.
//
// Why column-oriented and typed?
//   The supermarket CSV has 120,000 rows x 323 columns. Keeping every cell as a
//   std::string would be ~39 million strings (well over 1 GB). Here each column
//   is stored once in a compact form:
//     - numbers, dates and times -> one double per row
//     - text                     -> one 4-byte code per row + a dictionary that
//                                   holds each distinct string only once
//   Typing also makes sorting correct: "10.5" sorts after "9.2", and dates sort
//   chronologically.
//
// How the sorting code uses it:
//   Rows never move. Each algorithm gets its own std::vector<int> of row
//   indices (identityOrder()) and rearranges that, using less()/compare() on the
//   chosen column. Both sorts therefore do exactly the same work per comparison,
//   which keeps the quicksort vs. mergesort comparison fair.
//
//   Dataset data;
//   data.load("datasets/supermarket_large_dataset.csv");
//   int col = data.columnIndex("total_amount");
//   std::vector<int> order = data.identityOrder();
//   // ... mergeSort(order, [&](int a, int b) { return data.less(col, a, b); });
//   std::string firstValue = data.cell(col, order[0]);

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum class ColumnType {
    Integer,   // whole numbers, e.g. customer_age
    Real,      // decimals, e.g. total_amount
    Date,      // "2023-07-01"          stored as 20230701
    Time,      // "06:20:09"            stored as seconds since midnight
    DateTime,  // "2023-07-01 06:20:09" stored as 20230701062009
    Text       // anything else, e.g. product_category
};

const char* columnTypeName(ColumnType type);

struct Column {
    std::string name;
    ColumnType type = ColumnType::Text;
    std::size_t missing = 0;               // number of blank cells

    // Integer, Real, Date, Time, DateTime: one value per row (NaN = blank).
    std::vector<double> numbers;

    // Text: codes[row] indexes into dictionary. Blank cells are the empty string.
    std::vector<std::uint32_t> codes;
    std::vector<std::string> dictionary;

    bool isText() const { return type == ColumnType::Text; }
};

class Dataset {
public:
    // Parses the whole CSV into memory. The first line must be the header.
    // If `onlyColumns` is non-empty, every other column is skipped while
    // parsing (saves memory if the GUI only exposes a few columns).
    // Throws std::runtime_error with a readable message on any problem.
    void load(const std::string& path, const std::vector<std::string>& onlyColumns = {});

    // ---- shape ----
    std::size_t numRows() const { return rows_; }
    std::size_t numColumns() const { return columns_.size(); }

    // ---- columns ----
    int columnIndex(const std::string& name) const;   // -1 if not found
    const std::string& columnName(int col) const { return columns_[col].name; }
    ColumnType columnType(int col) const { return columns_[col].type; }
    const Column& column(int col) const { return columns_[col]; }
    std::vector<std::string> columnNames() const;

    // ---- cells ----
    bool isMissing(int col, int row) const;
    double number(int col, int row) const;             // non-Text columns only
    const std::string& text(int col, int row) const;   // Text columns only
    std::string cell(int col, int row) const;          // any column, formatted for display

    // ---- ordering used by the sorting algorithms ----
    // Blank cells sort last; numbers/dates numerically; text alphabetically
    // (byte order). Returns <0, 0 or >0 like strcmp.
    int compare(int col, int rowA, int rowB) const;
    bool less(int col, int rowA, int rowB) const { return compare(col, rowA, rowB) < 0; }

    // 0, 1, 2, ..., numRows()-1 : the array each sort rearranges.
    std::vector<int> identityOrder() const;

    // ---- stats for the report ----
    double loadSeconds() const { return loadSeconds_; }
    std::size_t approxBytes() const;   // memory held by the parsed data

private:
    std::vector<Column> columns_;
    std::unordered_map<std::string, int> index_;   // column name -> column number
    std::size_t rows_ = 0;
    double loadSeconds_ = 0.0;
};
