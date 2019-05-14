/* 
 * File:   table_io_util.cpp
 * Implementation file for table_io_util.h.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include <string>
#include "Column.h"
#include "constants.h"
#include "InvalidQueryException.h"
#include "Row.h"
#include "Schema.h"
#include "string_util.h"
#include "table_io_util.h"

void table_io_util::formatColumnValue(const std::string& colType,
        std::string& colValue) {
    if (colType == "date") {
        Column col(colValue);
        colValue = boost::gregorian::to_iso_extended_string(col);
    } else if (colType == "time") {
        Column col(colValue);
        // Format of time string: YYYY-MM-DDTHH:MM:SS,fffffffff
        // We only want to extract the time part
        colValue = boost::posix_time::to_iso_extended_string(col).substr(11, 8);
    } else if (colType.find("char") != std::string::npos) {
        auto typeParts = string_util::split(colType, '(');
        colValue = string_util::getEscapedString(
                    string_util::extractQuoted(colValue));
        std::string type = typeParts[0];
        unsigned int limit = std::stoi(typeParts[1].substr(0,
                typeParts[1].size() - 1));
        if (colValue.size() > limit) {
            colValue.erase(colValue.size() - (colValue.size() - limit));
        } else if (colValue.size() < limit && type == "char") {
            colValue.append(limit - colValue.size(), ' ');
        }
    }
}

void table_io_util::validateReferencedColumn(const ColumnMetadata& metadata,
        const std::string& colValue) {
    if (metadata.getReferencedColumn() == "") {
        return;
    }
    auto referencedColParts = string_util::split(
            metadata.getReferencedColumn(), '.');
    auto table = referencedColParts[0];
    auto refColName = referencedColParts[1];
    std::ifstream tableFile(TABLE_DIRECTORY + table + TABLE_EXTENSION);
    std::string tmp;
    std::getline(tableFile, tmp);
    Row row(Schema(metadata.getTableName(), tmp));
    bool valid = false;
    while (!valid && tableFile >> row) {
        Column col = row.getColumn(refColName);
        if (!col.isNull() && static_cast<std::string> (col) == colValue) {
            valid = true;
        }
    }
    if (!valid) {
        throw InvalidQueryException("Value " + colValue
                + " does not reference " + metadata.getReferencedColumn());
    }
}

void table_io_util::validateReferencedBy(const ColumnMetadata& metadata,
        const std::string& oldValue, const fs::path& path) {
    auto colName = metadata.getColumnName();
    auto tableName = metadata.getTableName();
    std::string schemaStr;
    std::ifstream tableFile(path);
    std::getline(tableFile, schemaStr);
    Schema schema(path.stem(), schemaStr);
    for (const auto& otherMetadata : schema.getMetadataForColumns()) {
        auto refColName = otherMetadata.getReferencedColumn();
        if (refColName == tableName + "." + colName) {
            Row row(schema);
            bool valid = true;
            while (valid && tableFile >> row) {
                Column col =
                        row.getColumn(otherMetadata.getColumnName());
                if (!col.isNull()
                        && static_cast<std::string> (col) == oldValue) {
                    valid = false;
                }
            }
            if (!valid) {
                throw InvalidQueryException("Column " +
                    otherMetadata.getTableName() + "."
                    + otherMetadata.getColumnName() + " references a "
                    "value being modified or deleted");
            }
            break;
        }
    }
}

void table_io_util::validateReferencedBy(const ColumnMetadata& metadata,
        const std::string& oldValue) {
    for (const auto& dirEntry : fs::directory_iterator(TABLE_DIRECTORY)) {
        if (fs::is_regular_file(dirEntry.status())) {
            validateReferencedBy(metadata, oldValue, dirEntry.path());
        }
    }
}
