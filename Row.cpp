/* 
 * File:   Row.cpp
 * Implementation file for the Row class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include "Column.h"
#include "Row.h"
#include "string_util.h"
#include "InvalidQueryException.h"

std::istream& operator>>(std::istream& is, Row& row) {
    // Clear state information
    row.columns.clear();
    row.currentIndex = 0;
    std::string rowLine;
    std::getline(is, rowLine);
    std::istringstream rowScanner(rowLine);
    std::string colValue;
    unsigned int index = 0;
    while (rowScanner >> std::quoted(colValue)) {
        row.columns.push_back(Column(colValue, 
                row.schema.getMetadataForColumns()[index]));
        index++;
    }
    return is;
}

std::ostream& operator<<(std::ostream& os, const Row& row) {
    for (unsigned int i = 0; i < row.columns.size(); i++) {
        os << row.columns[i];
        if (i < row.columns.size() - 1) {
            os << " ";
        }
    }
    return os;
}

// Row::Row(const Schema& schema) implemented in header

Row::Row(const Schema& schema, const ColumnValues& values) {
    for (unsigned int i = 0; i < schema.getMetadataForColumns().size(); i++) {
        columns.push_back(Column(string_util::getEscapedString(values[i]), 
                schema.getMetadataForColumns()[i]));
    }
}


Row::~Row() {
    // No implementation needed
}

Column Row::getColumn(const std::string& colName) const {
    std::string colNameCopy = colName;
    std::string tableName;
    Column ret;
    if (colName.find('.') != std::string::npos) {
        auto parts = string_util::split(colName, '.', true);
        tableName = string_util::extractQuoted(parts[0]);
        colNameCopy = string_util::extractQuoted(parts[1]);
    }
    for (const auto& col : columns) {
        if (col.getMetadata().getColumnName() == colNameCopy) {
            if (tableName.empty() && ret) {
                throw InvalidQueryException("Ambiguous column: " + colNameCopy);
            }
            if (tableName.empty() 
                    || col.getMetadata().getTableName() == tableName) {
                ret = col;
            }
        }
    }
    if (ret) {
        return ret;
    }
    throw std::invalid_argument("Column " + colName + " does not exist");
}

ColumnVec Row::getColumns() const {
    return columns;
}

int Row::getColumnIndex(const std::string& colName) const {
    for (unsigned int i = 0; i < columns.size(); i++) {
        if (columns[i].getMetadata().getColumnName() == colName) {
            return i;
        }
    }
    return -1;
}

void Row::orderAndFilterColumns(const ColumnNames& colNames) {
    if (colNames.size() == 0) {
        return;
    }
    ColumnVec newColumns;
    for (const auto& name : colNames) {
        newColumns.push_back(getColumn(name));
    }
    columns = newColumns;
}

Column& Row::operator[](unsigned int index) {
    return columns[index];
}

Row& Row::operator>>(Column& col) {
    if (currentIndex == columns.size()) {
        currentIndex++;
        return *this;
    }
    col = columns[currentIndex++];
    return *this;
}

Row::operator bool() const {
    return currentIndex <= columns.size();
}

void Row::merge(const Row& otherRow) {
    checkInitialization();
    for (const auto& col : otherRow.columns) {
        columns.push_back(col);
    }
}

void Row::fillBlank(unsigned int count) {
    columns.clear();
    for (unsigned int i = 0; i < count; i++) {
        columns.push_back(Column("", schema.getMetadataForColumns()[i]));
    }
}


void Row::checkInitialization() const {
    if (columns.size() == 0) {
        throw std::logic_error("Row not initialized");
    }
}
