/* 
 * File:   Query.h
 * Header file for the Query class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef QUERY_H
#define QUERY_H

#include <string>
#include <unordered_map>
#include <vector>
#include "string_util.h"
#include "Result.h"
#include "ColumnMetadata.h"

using PropertyMap = std::unordered_map<std::string, std::string>;

/** 
 * A class that represents a SQL query. 
 * 
 * Queries have different types determined by the first word in the query 
 * string. The types supported by this database are enumerated in QueryType.
 * Based on the type of the query, it will have various properties. The
 * properties for each type are listed below.
 * 
 * CREATE\n
 *     tableName - The name of the table to create\n
 *     schema - The string representation of the schema of the table being
 *     created
 * 
 * DROP\n
 *     tableName - The name of the table to drop
 * 
 * UPDATE\n
 *     tableName - The name of the table to update\n
 *     columns - The columns being updated, separated by commas\n
 *     values - The values to update the columns to, separated by commas\n
 *     restrictions - The restrictions to apply when updating the columns
 * 
 * DELETE\n
 *     tableName - The name of the table to delete from\n
 *     restrictions - The restrictions to apply when deleting rows
 * 
 * INSERT\n
 *     tableName - The table to insert into\n
 *     columnNames - The names of the columns to populate, separated by commas\n
 *     columnValues - The values to insert, separated by commas
 * 
 * SELECT\n
 *     distinct - Defined if and only if the DISTINCT keyword was specified\n
 *     columnNames - The names of the columns to retrieve, separated by commas\n
 *     tableNames - The names of the tables to select from, 
 *         separated by commas\n
 *     restrictions - The restrictions to apply when selecting rows\n
 *     orderBy - The columns to order by (default "")\n
 *     desc - Defined if and only if the DESC keyword was specified
 */
class Query {
public:
    /** An enumeration of the types of queries supported by the database. */
    enum class QueryType {
        CREATE,
        DROP,
        UPDATE,
        DELETE,
        INSERT,
        SELECT
    };
    
    Query(const std::string& queryString);
    ~Query();
    
    /**
     * Gets a property of this query. Properties differ based on the type of the
     * query returned by getType() (see class documentation for more details).
     * 
     * @param propertyName The name of the property to get. Throws 
     * std::out_of_range if the property does not exist.
     * @return The value of the property.
     */
    std::string getProperty(const std::string& propertyName) const;
    
    /**
     * Determines if the query has a property matching the given name.
     * 
     * @param propertyName The name to search for
     * @return True if the query has the given property, false otherwise
     */
    bool hasProperty(const std::string& propertyName) const;
    
    /** Gets the type of this query. */
    QueryType getType() const;
    
    /** 
     * Executes the query and returns the result. If the query is invalid, 
     * throws std::invalid_argument.
     */
    Result execute() const;
    
private:
    std::string queryString;
    QueryType queryType;
    PropertyMap properties;
    
    /** Checks whether or not brackets in the query are balanced. */
    bool isBalanced() const;
    
    /** Parses the query, setting its type and associated properties. */
    void parse();
    /** Parses a CREATE query. */
    void parseCreateQuery();
    /** Parses a DROP query. */
    void parseDropQuery();
    /** Parses an INSERT query. */
    void parseInsertQuery();
    /** Parses an UPDATE query. */
    void parseUpdateQuery();
    /** Parses a DELETE query. */
    void parseDeleteQuery();
    /** Parses a SELECT query. */
    void parseSelectQuery();
    
    /**
     * Parses a primary key declaration.
     * 
     * @param parts The parts of the query, split on spaces.
     * @param metadataVec The metadata of the columns created so far
     * @param index The current index in parts. Will be modified by this
     * method.
     */
    void parsePrimaryKey(const std::vector<std::string>& parts, 
            MetadataVec& metadataVec, unsigned int& index);
};

#endif /* QUERY_H */

