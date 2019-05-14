/* 
 * File:   JoinedTable.cpp
 * Implementation file for the JoinedTable class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "InvalidQueryException.h"
#include "JoinedTable.h"
#include "string_util.h"
#include "Table.h"

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

JoinedTable::JoinedTable(const Table& table1, const Table& table2,
        const std::string& joinCondition) {
    assignBuildAndProbeTables(table1, table2);
    schema = probeTable->getSchema();
    schema.merge(buildTable->getSchema());
    if (joinCondition == "") {
        return;
    }
    auto parts = string_util::split(joinCondition, ' ', true);
    parseJoinCondition(parts);
}

JoinedTable::~JoinedTable() {
    // No implementation needed
}

JoinedTable& JoinedTable::operator>>(Row& row) {
    extractRow(row);
    if (row.getColumns().size() > 0) {
        row.orderAndFilterColumns(colFilter);
    }
    if (distinct) {
        std::string colValues;
        for (const auto& col : row.getColumns()) {
            colValues += col.getMetadata().getColumnName() + "="
                    + static_cast<std::string> (col) + ";";
        }
        if (columnsFound.find(colValues) == columnsFound.end()) {
            columnsFound.insert(colValues);
        } else {
            *this >> row;
        }
    }
    return *this;
}

void JoinedTable::insertRow(Row& row) {
    throw std::logic_error("Cannot insert rows in a joined table");
}

void JoinedTable::updateRows(UpdateMap& columnsToUpdate) {
    throw std::logic_error("Cannot update rows in a joined table");
}

void JoinedTable::deleteRows() {
    throw std::logic_error("Cannot delete rows in a joined table");
}

Table& JoinedTable::orderBy(const std::string& colNames, bool desc) {
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

JoinedTable::operator bool() const {
    return (tableStream && *tableStream) || (*buildTable && *probeTable);
}

unsigned int JoinedTable::getRowCount() const {
    return probeTable->getRowCount();
}

std::shared_ptr<Table> JoinedTable::clone() const {
    return std::make_shared<JoinedTable>(*this);
}

void JoinedTable::buildJoinMap(const ColNames& colNames) {
    buildTable->reset();
    Row row;
    while (*buildTable >> row) {
        for (const auto& name : colNames) {
            try {
                Column c = row.getColumn(name);
                joinMap[name + "=" + static_cast<std::string> (c)] = row;
            } catch (std::invalid_argument& e) {
            }
        }
    }
    buildTable->reset();
}

void JoinedTable::assignBuildAndProbeTables(const Table& table1, 
        const Table& table2) {
    if (table1.getRowCount() > table2.getRowCount()) {
        buildTable = table2.clone();
        probeTable = table1.clone();
    } else {
        buildTable = table1.clone();
        probeTable = table2.clone();
    }
}

void JoinedTable::parseJoinCondition(const std::vector<std::string>& parts) {
    ColumnNames buildTableColumns;
    for (unsigned int i = 0; i < parts.size();) {
        if (parts[i + 1] != "=") {
            throw InvalidQueryException("Joins currently only support the ="
                    " operator");
        }
        if (buildTable->getSchema().hasColumn(parts[i])) {
            columnMap[parts[i + 2]] = parts[i];
            buildTableColumns.push_back(parts[i]);
        } else {
            columnMap[parts[i]] = parts[i + 2];
            buildTableColumns.push_back(parts[i + 2]);
        }
        // Break on last join condition
        if (i == parts.size() - 3) {
            break;
        }
        i += 3;
    }
    buildJoinMap(buildTableColumns);
}

void JoinedTable::extractRow(Row& row) {
    do {
        if (tableStream) {
            row = Row(schema);
            *tableStream >> row;
            continue;
        }
        if (!(*probeTable >> row)) {
            break;
        }
        if (joinMap.size() == 0) {
            Row row2;
            if (!(*buildTable >> row2)) {
                buildTable->reset();
                *buildTable >> row2;
            }
            row.merge(row2);
        } else {
            extractRowJoined(row);
        }
    } while (!restriction.apply(row));
}

void JoinedTable::extractRowJoined(Row& row) {
    bool match = false;
    for (const auto& col : row.getColumns()) {
        std::string colName = col.getMetadata().getColumnName();
        // Ensure proper column name is used
        if (columnMap.find(colName) == columnMap.end()) {
            colName = col.getMetadata().getTableName() + "." + colName;
        }
        std::string colValue = static_cast<std::string> (col);
        if (columnMap.find(colName) != columnMap.end()
                && joinMap.find(columnMap[colName] + "=" + colValue)
                != joinMap.end()) {
            row.merge(joinMap[columnMap[colName] + "=" + colValue]);
            match = true;
            break;
        }
    }
    if (!match) {
        auto tmpRow = Row(buildTable->getSchema());
        tmpRow.fillBlank(buildTable->getSchema().getMetadataForColumns()
                .size());
        row.merge(tmpRow);
    }
}

