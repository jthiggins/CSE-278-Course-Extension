/* 
 * File:   Query.cpp
 * Implementation file for the Schema class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "Schema.h"
#include "string_util.h"
#include "ColumnMetadata.h"

using MetadataStrings = std::vector<std::string>;

Schema::Schema() {
    // No implementation needed
}

Schema::~Schema() {
    // No implementation needed
}

Schema::Schema(const std::string& tableName, const std::string& s) {
    auto tableNameCopy = tableName;
    if (tableName.find("http://") == 0) {
        tableNameCopy = tableName.substr(tableName.rfind("/") + 1);
    }
    MetadataStrings metadataStrings = string_util::split(s, '\t');
    for (const auto& colMetadataStr : metadataStrings) {
        ColumnMetadata colMetadata;
        std::istringstream is(colMetadataStr);
        is >> std::boolalpha >> colMetadata;
        colMetadata.tableName = tableNameCopy;
        addColumn(colMetadata);
    }
}

std::string Schema::toString() const {
    std::ostringstream os;
    os << std::boolalpha;
    unsigned int index = 0;
    for (const auto& colMetadata : metadata) {
        os << colMetadata;
        if (index < metadata.size() - 1) {
            os << "\t";
        }
        index++;
    }
    return os.str();
}

void Schema::addColumn(const ColumnMetadata& colMetadata) {
    metadata.push_back(colMetadata);
}

int Schema::getColumnIndex(const std::string& colName) const {
    for (unsigned int i = 0; i < metadata.size(); i++) {
        if (metadata[i].getColumnName() == colName) {
            return i;
        }
    }
    return -1;
}


bool Schema::hasColumn(const std::string& colName) const {
    std::string colNameCopy = colName;
    std::string tableName;
    if (colName.find('.') != std::string::npos) {
        auto parts = string_util::split(colName, '.', true);
        tableName = parts[0];
        colNameCopy = parts[1];
    }
    for (const auto& colMetadata : metadata) {
        if (colMetadata.getColumnName() == colNameCopy) {
            if (tableName.empty() || colMetadata.getTableName() == tableName) {
                return true;
            }
        }
    }
    return false;
}

const MetadataVec Schema::getMetadataForColumns() const {
    return metadata;
}

ColumnMetadata Schema::getColumnMetadata(const std::string& colName) const {
    for (const auto& colMetadata : metadata) {
        if (colMetadata.getColumnName() == colName)
            return colMetadata;
    }
    throw std::invalid_argument("Column " + colName + " does not exist");
}

void Schema::merge(const Schema& schema) {
    for (const auto& colMetadata : schema.metadata) {
        metadata.push_back(colMetadata);
    }
}

