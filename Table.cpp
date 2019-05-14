/* 
 * File:   Table.cpp
 * Implementation file for the Table class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#include <boost/asio.hpp>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <string>
#include <vector>
#include "constants.h"
#include "InvalidQueryException.h"
#include "JoinedTable.h"
#include "Restriction.h"
#include "Row.h"
#include "string_util.h"
#include "Table.h"
#include "table_io_util.h"

// Helper functions
namespace {
    /**
     * Compares rows when ordering them.
     */
    bool compareRows(const Row& row1, const Row& row2, 
            const ColumnNames& nameVec, bool desc) {
        for (const auto& colName : nameVec) {
            auto col1 = row1.getColumn(colName);
            auto col2 = row2.getColumn(colName);
            if (col1 == col2) {
                continue;
            }
            return desc ? (col1 > col2) : (col1 < col2);
        }
        return false;
    }
}  // namespace

Table::Table() : restriction(Restriction("")) {
    // No implementation needed
}

Table::Table(const std::string& tableName, const Schema& schema)
: schema(schema), tableName(tableName), restriction(Restriction("")) {
    tableStream = std::make_shared<std::fstream>
        (TABLE_DIRECTORY + tableName + TABLE_EXTENSION);
    if (!tableStream->good())
        hasRows = false;
    if (!isFromURL) {
        countRows();
    }
}

Table::Table(const std::shared_ptr<std::iostream>& tableStream,
        const std::string& tableName, const Schema& schema)
        : schema(schema), tableName(tableName),
          restriction(Restriction("")), tableStream(tableStream) {
    if (!tableStream->good()) {
        hasRows = false;
    }
    if (tableName.find("http://") == 0) {
        isFromURL = true;
        rowCount = std::numeric_limits<int>::max();
        this->tableName = tableName.substr(tableName.rfind("/") + 1);
    } else {
        countRows();
    }
}

Table::~Table() {
    // No implementation needed
}

Table& Table::operator>>(Row& row) {
    if (tableStream->tellg() == 0) {
        // Skip schema header
        std::string schema;
        std::getline(*tableStream, schema);
    }
    row = Row(schema);
    extractRow(row);
    return *this;
}

Table::operator bool() const {
    return hasRows;
}

const Schema Table::getSchema() const {
    return schema;
}

void Table::insertRow(Row& row) {
    if (isFromURL) {
        return;
    }
    for (unsigned int index = 0; index < row.getColumns().size(); index++) {
        ColumnMetadata metadata = row[index].getMetadata();
        std::string colValue = row[index];
        validateColumnValue(metadata, colValue, index);
        table_io_util::formatColumnValue(metadata.getColumnType(), colValue);
        row[index] = Column(colValue);
    }
    std::fstream::pos_type original = tableStream->tellg();
    // Go to end of file
    tableStream->seekg(0, tableStream->end);
    *tableStream << row << std::endl;
    // Reset file pointer
    tableStream->seekg(original);
    rowCount++;
}

void Table::updateRows(UpdateMap& columnsToUpdate) {
    if (isFromURL) {
        throw InvalidQueryException("Cannot update a remote table");
    }
    for (auto& entry : columnsToUpdate) {
        auto metadata = schema.getColumnMetadata(entry.first);
        if (metadata.isPrimaryKey() && restriction.isEmpty()) {
            throw InvalidQueryException("Primary key must be unique");
        }
        validateColumnValue(metadata, entry.second,
                schema.getColumnIndex(entry.first));
        table_io_util::formatColumnValue(metadata.getColumnType(),
                entry.second);
    }
    auto original = tableStream->tellg();
    tableStream->seekg(0);
    std::string schemaStr;
    std::getline(*tableStream, schemaStr);
    writeUpdatedRows(columnsToUpdate);
    tableStream->seekg(original);
}

void Table::deleteRows() {
    if (isFromURL) {
        throw InvalidQueryException("Cannot delete from a remote table");
    }
    auto original = tableStream->tellg();
    tableStream->seekg(0);
    std::string schemaStr;
    std::getline(*tableStream, schemaStr);
    writeUndeletedRows();
    tableStream->seekg(original);
    rowCount--;
}

Table& Table::filterColumnsByName(const std::string& colNames) {
    if (colNames == "") {
        colFilter.clear();
        return *this;
    }
    if (colNames != "*") {
        colFilter = string_util::split(colNames, ',');
    }
    return *this;
}

Table& Table::filterDistinct(bool distinct) {
    this->distinct = distinct;
    return *this;
}

Table& Table::orderBy(const std::string& colNames, bool desc) {
    if (colNames.empty()) {
        return *this;
    }
    ColumnNames nameVec = string_util::split(colNames, ',');
    std::vector<Row> rows;
    Row row;
    while (*this >> row) {
        rows.push_back(row);
    }
    std::sort(rows.begin(), rows.end(), [&](Row row1, Row row2) {
        return compareRows(row1, row2, nameVec, desc);
    });
    tableStream = std::make_shared<std::stringstream>();
    *tableStream << schema.toString() << std::endl;
    unsigned int index = 0;
    for (const auto& row : rows) {
        *tableStream << row;
        if (index != rows.size() - 1) {
            *tableStream << std::endl;
        }
        index++;
    }
    hasRows = true;
    return *this;
}

Table& Table::setRestrictions(const std::string& restrictions) {
    restriction = Restriction(restrictions);
    return *this;
}

JoinedTable Table::joinTo(Table& other,
        const std::string& joinCondition) {
    return JoinedTable(*this, other, joinCondition);
}

void Table::reset() {
    tableStream->clear();
    tableStream->seekg(0);
    hasRows = true;
}

unsigned int Table::getRowCount() const {
    return rowCount;
}

std::shared_ptr<Table> Table::clone() const {
    return std::make_shared<Table>(*this);
}

void Table::validateColumnValue(const ColumnMetadata& metadata,
        const std::string& colValue, const unsigned int indexInSchema) {
    std::string colName = metadata.getColumnName();
    validateDataType(colName, metadata.getColumnType(), colValue);
    if (metadata.isNotNull() && colValue == Column::NULL_VALUE) {
        throw InvalidQueryException(colName + " cannot be null");
    } else if (colValue == Column::NULL_VALUE) {
        return;
    }
    if (metadata.isPrimaryKey()) {
        checkForDuplicateValue(colValue, indexInSchema);
    }
    table_io_util::validateReferencedColumn(metadata, colValue);
}

void Table::validateDataType(const std::string& colName,
        const std::string& dataType, const std::string& value) {
    if (value == Column::NULL_VALUE) {
        return;
    }
    Column col(value);
    try {
        if (dataType == "int") {
            static_cast<int> (col);
        } else if (dataType == "long") {
            static_cast<long long> (col);
        } else if (dataType == "float") {
            static_cast<float> (col);
        } else if (dataType == "double") {
            static_cast<double> (col);
        } else if (dataType == "date") {
            col.operator boost::gregorian::date();
        } else if (dataType == "time") {
            col.operator boost::posix_time::ptime();
        } else if (dataType.find("char") != std::string::npos) {
            if (string_util::extractQuoted(value) == value) {
                throw std::exception();
            }
        }
    } catch (const std::exception& e) {
        throw InvalidQueryException("Invalid data type: expected "
                + dataType + " for column " + colName);
    }
}

void Table::checkForDuplicateValue(const std::string& value,
        const unsigned int index) {
    std::fstream::pos_type returnPos = tableStream->tellg();
    std::string line;
    while (std::getline(*tableStream, line)) {
        std::istringstream is(line);
        std::string column;
        for (unsigned int i = 0; i <= index; i++) {
            is >> std::quoted(column);
        }
        if (column == value) {
            throw InvalidQueryException("Primary key must be unique");
        }
    }
    // Clear eof bit on file
    tableStream->clear();
    // Return file read position to original location
    tableStream->seekg(returnPos);
}

void Table::countRows() {
    std::string line;
    if (tableStream->tellg() == 0) {
        std::getline(*tableStream, line);
    }
    while (std::getline(*tableStream, line)) {
        rowCount++;
    }
    reset();
}

void Table::writeUpdatedRows(const UpdateMap& columnsToUpdate) {
    std::string tableStreamPath = TABLE_DIRECTORY + tableName + TABLE_EXTENSION;
    std::string tmpFilePath = TABLE_DIRECTORY + tableName + TEMP_EXTENSION;
    std::ofstream out(tmpFilePath);
    out << schema.toString() << std::endl;
    Row row(schema);
    while (*tableStream >> row) {
        if (!restriction.apply(row)) {
            out << row << std::endl;
            continue;
        }
        for (const auto& col : row.getColumns()) {
            std::string colName = col.getMetadata().getColumnName();
            if (columnsToUpdate.find(colName) != columnsToUpdate.end()) {
                table_io_util::validateReferencedBy(col.getMetadata(), col);
                out << std::quoted(columnsToUpdate.at(colName)) << " ";
            } else {
                out << std::quoted(static_cast<std::string> (col)) << " ";
            }
        }
        out << std::endl;
    }
    std::rename(tmpFilePath.c_str(), tableStreamPath.c_str());
}

void Table::writeUndeletedRows() {
    std::string tableStreamPath = TABLE_DIRECTORY + tableName + TABLE_EXTENSION;
    std::string tmpFilePath = TABLE_DIRECTORY + tableName + TEMP_EXTENSION;
    std::ofstream out(tmpFilePath);
    out << schema.toString() << std::endl;
    Row row(schema);
    while (*tableStream >> row) {
        if (!restriction.apply(row)) {
            out << row << std::endl;
        } else {
            for (const auto& col : row.getColumns()) {
                try {
                    table_io_util::validateReferencedBy(col.getMetadata(), col);
                } catch (std::exception& e) {
                    std::remove(tmpFilePath.c_str());
                    throw;
                }
            }
        }
    }
    std::rename(tmpFilePath.c_str(), tableStreamPath.c_str());
}

void Table::extractRow(Row& row) {
    while (hasRows) {
        do {
            if (!(*tableStream >> row)) {
                hasRows = false;
                break;
            }
        } while (!restriction.apply(row));
        if (hasRows) {
            row.orderAndFilterColumns(colFilter);
        }
        if (!distinct) {
            break;
        } else {
            std::string colValues;
            for (const auto& col : row.getColumns()) {
                colValues += col.getMetadata().getColumnName() + "="
                        + static_cast<std::string> (col) + ";";
            }
            if (columnsFound.find(colValues) == columnsFound.end()) {
                columnsFound.insert(colValues);
                break;
            }
        }
    }
}
