/* 
 * File:   ColumnMetadata.cpp
 * Implementation file for the ColumnMetadata class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */
#include <iostream>
#include <iomanip>
#include <string>
#include "ColumnMetadata.h"

std::ostream& operator<<(std::ostream& os, const ColumnMetadata& metadata) {
    os << std::quoted(metadata.colName) << " " << std::quoted(metadata.colType)
       << " "  << std::quoted(metadata.references) << " " 
       << metadata.primaryKey << " "  << metadata.notNull;
    return os;
}

std::istream& operator>>(std::istream& is, ColumnMetadata& metadata) {
    is >> std::quoted(metadata.colName) >> std::quoted(metadata.colType)
       >> std::quoted(metadata.references) >> metadata.primaryKey 
       >> metadata.notNull;
    return is;
}

ColumnMetadata::ColumnMetadata() {
    // No implementation needed
}

ColumnMetadata::~ColumnMetadata() {
    // No implementation needed
}

std::string ColumnMetadata::getColumnName() const {
    return colName;
}

std::string ColumnMetadata::getTableName() const {
    return tableName;
}

std::string ColumnMetadata::getColumnType() const {
    return colType;
}

std::string ColumnMetadata::getReferencedColumn() const {
    return references;
}

bool ColumnMetadata::isPrimaryKey() const {
    return primaryKey;
}

bool ColumnMetadata::isNotNull() const {
    return notNull;
}
