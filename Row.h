/* 
 * File:   Row.h
 * Header file for the Row class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef ROW_H
#define ROW_H

#include <iostream>
#include <string>
#include <vector>
#include "Column.h"
#include "Schema.h"

using ColumnVec = std::vector<Column>;
using ColumnNames = std::vector<std::string>;
using ColumnValues = std::vector<std::string>;

/**
 * Represents a row in a table. Columns can be accessed by name using
 * the index operator.
 */
class Row {
public:
    friend std::istream& operator>>(std::istream& is, Row& row);
    friend std::ostream& operator<<(std::ostream& os, const Row& row);
    Row() {}
    Row(const Schema& schema) : schema(schema) {}
    Row(const Schema& schema, const ColumnValues& values);
    ~Row();
    
    /**
     * Gets the column with the given name.
     * @param colName The name of the column to search for
     * @return The column with the given name
     */
    Column getColumn(const std::string& colName) const;
    
    /**
     * Gets the columns in the row.
     */
    ColumnVec getColumns() const;
    
    /**
     * Gets the index of the column with the given name.
     * @param colName The name of the column to search for
     * @return The column's index, or -1 if the column was not found
     */
    int getColumnIndex(const std::string& colName) const;
    
    /**
     * Changes the columns stored in the row to only match the columns with
     * the provided names, in the order they are given.
     * 
     * @param colNames The names of the columns to order and filter by
     */
    void orderAndFilterColumns(const ColumnNames& colNames);
    
    /**
     * Gets the column at the given index.
     * @param index The index to look at
     * @return The column at the given index
     */
    Column& operator[](unsigned int index);
    
    /**
     * Extracts the next column from this row.
     * @param col The column to insert into
     */
    Row& operator>>(Column& col);
    
    /** Used for performing streamlike operations on the row. */
    operator bool() const;
    
    /** Merges this row with the given row. */
    void merge(const Row& otherRow);
    
    /**
     * Fills this row with blank columns.
     * 
     * @param count The number of columns to insert
     */
    void fillBlank(unsigned int count);
    
private:
    ColumnVec columns;
    Schema schema;
    unsigned int currentIndex = 0;
    
    /**
     * Checks that this row has been initialized with a schema.
     */
    void checkInitialization() const;  
};

#endif /* ROW_H */

