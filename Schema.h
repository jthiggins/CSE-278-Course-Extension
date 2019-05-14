/* 
 * File:   Schema.h
 * Header file for the Schema class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef SCHEMA_H
#define SCHEMA_H

#include <string>
#include <vector>
#include "ColumnMetadata.h"

using MetadataVec = std::vector<ColumnMetadata>;

/**
 * Represents the result of a query. This class can be used to extract rows
 * returned by the query.
 */
class Schema {
public:
    Schema();
    ~Schema();
    
    /** Loads a schema from a string. */
    Schema(const std::string& tableName, const std::string& s);
    
    /** Converts the schema to a string. */
    std::string toString() const;
    
    /** Adds the provided column to the schema. */
    void addColumn(const ColumnMetadata& colMetadata);
    
    /** 
     * Gets the index for the given column. Returns -1 if the column is not 
     * found.
     */
    int getColumnIndex(const std::string& colName) const;
    
    /** Checks if the schema contains a column with the given name. */
    bool hasColumn(const std::string& colName) const;
    
    /** Returns the metadata for the columns in the schema. */
    const MetadataVec getMetadataForColumns() const;
    
    /** Gets the metadata for the given column. */
    ColumnMetadata getColumnMetadata(const std::string& colName) const;
    
    /** Merges this schema with the given schema. */
    void merge(const Schema& schema);
    
private:
    MetadataVec metadata;
};

#endif /* SCHEMA_H */

