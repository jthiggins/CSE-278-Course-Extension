/* 
 * File:   ColumnMetadata.h
 * Header file for the ColumnMetadata class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef COLUMNMETADATA_H
#define COLUMNMETADATA_H

#include <iostream>
#include <string>

/**
 * A class to store the metadata for a column. Used for ensuring valid 
 * operations on data in the database.
 */
class ColumnMetadata {
public:
    friend std::ostream& operator<<(std::ostream& os, 
                                    const ColumnMetadata& metadata);
    friend std::istream& operator>>(std::istream& is, ColumnMetadata& metadata);
    
    friend class Schema;
    friend class Query;
    
    ColumnMetadata();
    
    ColumnMetadata(const std::string& colName, const std::string& tableName,
                   const std::string& colType, const std::string& references, 
                   bool isPrimaryKey, bool notNull) : 
                        colName(colName), tableName(tableName), 
                        colType(colType), references(references), 
                        primaryKey(isPrimaryKey),
                        notNull(primaryKey ? true : notNull) {}
    ~ColumnMetadata();
    
    // Getter methods
    std::string getColumnName() const;
    std::string getTableName() const;
    std::string getColumnType() const;
    std::string getReferencedColumn() const;
    bool isPrimaryKey() const;
    bool isNotNull() const;
    
private:
    std::string colName, tableName, colType, references;
    bool primaryKey, notNull;
};

#endif /* COLUMNMETADATA_H */

