/* 
 * File:   Table.h
 * Header file for the Table class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef TABLE_H
#define TABLE_H

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Restriction.h"
#include "Row.h"
#include "Schema.h"

using UpdateMap = std::unordered_map<std::string, std::string>;

class JoinedTable;  // Forward declaration required due to circular dependencies

/**
 * Represents a table in the database. Every table has an associated schema
 * used to determine column names, data types, primary keys, and so on. Rows
 * can be extracted from a table using the extraction operator.
 */
class Table {
public:
    Table();
    Table(const std::string& tableName, const Schema& schema);
    Table(const std::shared_ptr<std::iostream>& tableStream, 
          const std::string& tableName, const Schema& schema);
    virtual ~Table();

    /**
     * Extracts the next row from the table.
     * @param row The row to store
     */
    virtual Table& operator>>(Row& row);

    /**
     * Gets the schema associated with this table.
     */
    virtual const Schema getSchema() const;

    /**
     * Inserts the given row into the table.
     * 
     * @param row The row to insert
     * @param lineNum The number of the line to overwrite
     */
    virtual void insertRow(Row& row);

    /**
     * Updates the rows in the table. IMPORTANT: this function will update ALL
     * rows in the table. If you need to update specific rows, use
     * withRestrictions() first!
     * 
     * @param columnsToUpdate A map of column names to updated values
     */
    virtual void updateRows(UpdateMap& columnsToUpdate);
    
    /**
     * Deletes all rows in the table. Note: if only certain rows should be
     * deleted, they must be restricted using withRestrictions() before 
     * deleting them!
     */
    virtual void deleteRows();
    
    /**
     * Tells the table to filter the columns in the rows retrieved from 
     * the table. The columns extracted from the table after applying this 
     * method will be extracted in the order they are listed in colNames.
     * NOTE: Calling this method will erase unlisted columns from the table.
     * If you need access to those columns, this method should be called after
     * accessing them.
     * 
     * @param colNames The names of the columns to extract in the order they
     * are to be extracted in, separated by commas
     */
    Table& filterColumnsByName(const std::string& colNames);
    
    /**
     * If distinct is true, tells the table to filter out rows where all columns
     * are duplicates of the columns in a row extracted beforehand.
     * 
     * @param distinct Whether or not the filter should be applied
     */
    Table& filterDistinct(bool distinct);
    
    /**
     * Orders the rows in the table by the given column names.
     * 
     * @param colNames The names of the columns to sort by
     * @param desc Whether or not the rows should be sorted in descending order
     */
    virtual Table& orderBy(const std::string& colNames, bool desc);

    /**
     * Adds constraints to the table that filter out the rows that are returned.
     * 
     * @param restrictions The restrictions to put on the table, in the form
     * you would find in a SQL WHERE clause.
     * 
     * @return The table with the given constraints.
     */
    virtual Table& setRestrictions(const std::string& restrictions);
    
    /**
     * Joins this table to another table.
     * 
     * @param other The table to join to
     * @param joinCondition The condition to join on
     * @return A JoinedTable representing the result of joining the tables
     */
    virtual JoinedTable joinTo(Table& other, 
        const std::string& joinCondition);

    /** Used for extracting rows in while loops. */
    virtual operator bool() const;
    
    /**
     * Resets the table so that it begins pulling rows from the beginning again.
     */
    void reset();
    
    /** Gets the number of rows in the table. */
    virtual unsigned int getRowCount() const;
    
    /** Creates a shared_ptr pointing to a new copy of this table. */
    virtual std::shared_ptr<Table> clone() const;

protected:
    Schema schema;
    bool hasRows = true;
    bool isFromURL = false;
    bool distinct = false;
    std::string tableName;  // Used for copying tables
    std::vector<std::string> colFilter;  // Used to filter columns
    std::unordered_set<std::string> columnsFound;  // Used to filter duplicates
    Restriction restriction;  // Used for WHERE clauses
    unsigned int rowCount = 0;
    std::shared_ptr<std::iostream> tableStream;
    
private:
    /**
     * Ensures that not null, primary key, and reference conditions are met for 
     * the given column value.
     * 
     * @param metadata The metadata for the column whose value is being 
     * validated
     * @param colValue The value to validate
     * @param indexInFile The index of the column in the table's schema
     * @throw InvalidQueryException if one or more conditions are not met
     */
    void validateColumnValue(const ColumnMetadata& metadata, 
            const std::string& colValue, const unsigned int indexInSchema);
    
    /**
     * Validates the data type based on the column value.
     * 
     * @param colName The name of the column whose value is being validated.
     * Used for error messages.
     * @param dataType The data type to validate
     * @param value The value to validate the data type for
     */
    void validateDataType(const std::string& colName,
            const std::string& dataType, const std::string& value);
    
    /**
     * Checks to see if a duplicate value exists in the table. This function
     * is used to check for the uniqueness of a primary key.
     * 
     * @param value The column value to compare against
     * @param index The index of the column in the table's schema
     * @throw InvalidQueryException if a duplicate value exists
     */
    void checkForDuplicateValue(const std::string& value, 
            const unsigned int index);
    
    /**
     * Counts the rows in the table.
     */
    void countRows();
    
    /**
     * Writes updated rows into the temporary table, then replaces the table
     * file.
     * 
     * @param See updateRows().
     */
    void writeUpdatedRows(const UpdateMap& columnsToUpdate);
    
    /**
     * Writes undeleted rows into the temporary table, then replaces the table
     * file.
     */
    void writeUndeletedRows();
    
    /** Extracts the next row from the table. */
    void extractRow(Row& row);
};

#endif /* TABLE_H */

