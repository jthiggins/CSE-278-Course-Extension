/* 
 * File:   Query.cpp
 * Implementation file for the Query class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>
#include "ColumnMetadata.h"
#include "constants.h"
#include "InvalidQueryException.h"
#include "Query.h"
#include "Result.h"
#include "Schema.h"
#include "string_util.h"

using QueryParts = std::vector<std::string>;
using StringVec = std::vector<std::string>;

// Helper functions
namespace {

    /**
     * Formats the given query string to prepare it for parsing.
     * 
     * @param query The query string
     */
    void formatQuery(std::string& query) {
        std::string charsToSeparate = "(,);=";
        std::ostringstream os;
        char quoteChar = '\0';
        bool escaped = false, quoted = false, whitespaceFound = false;
        for (const auto& c : query) {
            if (c == '\\') {
                escaped = !escaped;
                os << c;
            } else {
                // Trim extra spaces
                if (c == ' ' && whitespaceFound && !quoted) {
                    continue;
                }
                if ((c == '"' || c == '\'') && !escaped) {
                    if (quoteChar == '\0' || quoteChar == c) {
                        quoted = !quoted;
                        quoteChar = (quoteChar == '\0' ? c : '\0');
                    }
                }
                escaped = false;
                if (!quoted && (charsToSeparate.find(c) != std::string::npos)) {
                    os << (whitespaceFound ? "" : " ") << c
                            << (c != ';' ? " " : "");
                } else {
                    os << c;
                }
            }
            whitespaceFound = (os.str().back() == ' ');
        }
        query = os.str();
        query = string_util::replace(query, "< =", "<=");
        query = string_util::replace(query, "> =", ">=");
        query = string_util::replace(query, "! =", "!=");
    }

    /** 
     * Makes sure the given column metadata is valid in the context of the
     * table's schema (i.e., there is only one primary key and column names are
     * unique).
     * 
     * @param metadata The metadata to check
     * @param colNames A set to hold the names of the columns in the table
     * @param primaryKeyFound Whether or not a primary key for the table has
     * been seen. This value will be updated by this function.
     * @throw InvalidQueryException if the metadata is not valid
     */
    void ensureValidMetadata(const ColumnMetadata& metadata,
            std::set<std::string>& colNames,
            bool& primaryKeyFound) {
        // Cannot have more than one primary key
        if (metadata.isPrimaryKey()) {
            if (primaryKeyFound) {
                throw InvalidQueryException("Table cannot have more than one"
                        " primary key");
            }
            primaryKeyFound = true;
        }
        // Column names must be unique
        if (colNames.find(metadata.getColumnName()) != colNames.end()) {
            throw InvalidQueryException("Column names must be unique");
        }
        colNames.insert(metadata.getColumnName());
    }

    /**
     * Makes sure the given data type is supported by the database.
     * 
     * @param dataType The data type to check
     * @throw InvalidQueryException if the data type is invalid
     */
    void checkDataType(const std::string& dataType) {
        if (dataType != "int" && dataType != "bigint" && dataType != "float"
                && dataType != "double" && dataType != "date"
                && dataType != "time") {
            // Check for parenthesized data types (e.g. varchar and char)
            std::regex pattern("char\\(\\d+\\)|varchar\\(\\d+\\)");
            if (!std::regex_match(dataType, pattern)) {
                throw InvalidQueryException("Invalid data type " + dataType);
            }
        }
    }

    /**
     * Gets the options for a column (e.g. if the column is a primary key or is
     * not null) from a query.
     * 
     * @param parts The parts of the query string, split by spaces
     * @param index The current index being examined. This value will be
     * updated by this function.
     * @param colName The name of the column
     * @param references A string to store the name of the column referenced
     * by this column
     * @param notNull A boolean to store whether or not this column can contain
     * null values
     */
    void extractColumnOptions(const QueryParts& parts, unsigned int& index,
            const std::string& colName,
            std::string& references, bool& notNull) {
        while (parts[index] != "," && index < parts.size() - 2) {
            if (string_util::toLowercase(parts[index]) == "not") {
                if (string_util::toLowercase(parts[index + 1]) == "null") {
                    notNull = true;
                    index += 2;
                } else {
                    throw InvalidQueryException("Expected 'null' for column "
                            + colName);
                }
            } else if (string_util::toLowercase(parts[index]) == "references") {
                if (parts[index + 1] == "(" && parts[index + 3] == ")") {
                    references = parts[index + 2];
                    index += 4;
                } else {
                    throw InvalidQueryException("Missing brackets for column "
                            + colName);
                }
            } else {
                throw InvalidQueryException("Unexpected symbol " + parts[index]
                        + " for column " + colName);
            }
        }
    }

    /**
     * Creates the metadata for the next column in the query.
     * 
     * @param parts The parts of the query string separated by spaces
     * @param index The current location being examined in parts. This value
     * will be updated by this function.
     * @return The metadata for the next column in the query
     */
    ColumnMetadata createColumnMetadata(const std::string& tableName,
            const std::vector<std::string>& parts, unsigned int& index) {
        std::string colName = parts[index++];
        std::string dataType = string_util::toLowercase(parts[index++]);
        if (parts[index] == "(") {
            dataType += parts[index] + parts[index + 1] + parts[index + 2];
            index += 3;
        }
        checkDataType(dataType);
        std::string references;
        bool isPrimaryKey = false, notNull = false;
        extractColumnOptions(parts, index, colName, references, notNull);
        index++;
        return ColumnMetadata(colName, tableName, dataType, references,
                isPrimaryKey, notNull);
    }

    /**
     * Populates the column properties for an INSERT query.
     * 
     * @param properties The property map to modify
     * @param parts A vector of parts of the query separated by spaces
     */
    void populateInsertColumnProperties(PropertyMap& properties,
            QueryParts& parts) {
        unsigned int index = 4;
        properties["columnNames"] = "";
        while (parts[index] != ")") {
            properties["columnNames"] += parts[index];
            index++;
        }
        if (index == parts.size() - 1
                || string_util::toLowercase(parts[index + 1]) != "values") {
            throw InvalidQueryException("Expected 'values' after column "
                    "declarations");
        }
        if (parts[index + 2] != "(") {
            throw InvalidQueryException("Expected value declarations within "
                    "parentheses");
        }
        index += 3;
        properties["columnValues"] = "";
        while (parts[index] != ")") {
            if (string_util::toLowercase(parts[index]) == "null") {
                parts[index] = Column::NULL_VALUE;
            }
            properties["columnValues"] += parts[index];
            index++;
        }
    }

    /**
     * Parses the restrictions out of the given query.
     * 
     * @param parts The parts of the query, split on spaces
     * @param index The current index in parts. Will be modified by this 
     *      function.
     * @return The restrictions given in the query
     */
    std::string parseRestrictions(const QueryParts& parts,
            unsigned int& index) {
        std::string restrictions = "";
        if (string_util::toLowercase(parts.at(index)) == "where") {
            index++;
            while (parts.at(index) != ";"
                    && string_util::toLowercase(parts.at(index)) != "order") {
                restrictions += parts.at(index++) + " ";
            }
            // Erase last space
            restrictions.erase(restrictions.length() - 1);
        } else if (parts.at(index) != ";"
                && string_util::toLowercase(parts.at(index)) != "order") {
            throw InvalidQueryException("Malformed query");
        }
        return restrictions;
    }

    /**
     * Parses the ORDER BY section of a SELECT query.
     * 
     * @param parts The parts of the query, split on spaces.
     * @param index The current index in parts. Will be modified by this 
     *      function.
     * @return The ORDER BY section in the query
     */
    std::string parseOrderBy(const QueryParts& parts,
            unsigned int& index) {
        std::string ret = "";
        if (string_util::toLowercase(parts.at(index)) == "order") {
            if (string_util::toLowercase(parts.at(++index)) != "by") {
                throw InvalidQueryException("Expected 'by' after 'order'");
            }
            index++;
            while (string_util::toLowercase(parts.at(index)) != ";" &&
                    string_util::toLowercase(parts.at(index)) != "desc") {
                ret += parts.at(index++);
            }
        }
        return ret;
    }

    /**
     * Determines if the given string is a column name.
     */
    bool isColumnName(const std::string& s) {
        if (s.at(0) == '"') {
            return false;
        }
        try {
            std::stod(s);
        } catch (std::exception& e) {
            return true;
        }
        return false;
    }

    /**
     * Extracts the join conditions from the restrictions (WHERE clause) in the
     * query.
     * @param properties The properties of the query
     * @return The join conditions of the query
     */
    std::string extractJoinConditions(const PropertyMap& properties) {
        if (properties.at("restrictions") == "") {
            return "";
        }
        auto parts = string_util::split(properties.at("restrictions"), ' ',
                true);
        std::string joinConditions;
        for (unsigned int index = 0; index < parts.size();) {
            auto left = parts[index], right = parts[index + 2];
            bool updateJoinConditions = isColumnName(left)
                    && isColumnName(right);
            if (updateJoinConditions) {
                joinConditions += left + " " + parts[index + 1] + " " + right;
            }
            index += 3;
            if (index < parts.size() && (parts[index] == "and"
                    || parts[index] == "or")) {
                joinConditions += (updateJoinConditions ? " " : "");
                index++;
            }
        }
        if (joinConditions.at(joinConditions.length() - 1) == ' ') {
            joinConditions.erase(joinConditions.length() - 1);
        }
        return joinConditions;
    }
}  // namespace

Query::Query(const std::string& queryString) {
    this->queryString = queryString;
    formatQuery(this->queryString);
    parse();
}

Query::~Query() {
    // No implementation needed
}

Result Query::execute() const {
    return Result(*this);
}

std::string Query::getProperty(const std::string& propertyName) const {
    return properties.at(propertyName);
}

bool Query::hasProperty(const std::string& propertyName) const {
    return properties.find(propertyName) != properties.end();
}

Query::QueryType Query::getType() const {
    return queryType;
}

bool Query::isBalanced() const {
    std::stack<char> bracketStack;
    bool ignore = false;  // Used to ignore quoted content
    bool escaped = false;  // Used to properly parse escaped characters
    for (const auto& c : queryString) {
        if (c == '"' && !escaped) {
            ignore = !ignore;
        }
        escaped = (c == '\\' ? !escaped : false);
        if (ignore) {
            continue;
        }
        if (c == '(') {
            bracketStack.push(c);
        } else if (c == ')') {
            if (bracketStack.size() == 0) {
                return false;
            }
            bracketStack.pop();
        }
    }
    return bracketStack.size() == 0
            && string_util::split(queryString, '"', true).size() % 2 == 1
            && string_util::split(queryString, '\'', true).size() % 2 == 1;
}

void Query::parse() {
    if (queryString.find(";") != queryString.length() - 1) {
        throw InvalidQueryException("Missing semicolon at end");
    } else if (!isBalanced()) {
        throw InvalidQueryException("Unbalanced parentheses or quotes");
    }
    if (string_util::toLowercase(queryString).find("create") == 0) {
        queryType = QueryType::CREATE;
        parseCreateQuery();
    } else if (string_util::toLowercase(queryString).find("drop") == 0) {
        queryType = QueryType::DROP;
        parseDropQuery();
    } else if (string_util::toLowercase(queryString).find("insert") == 0) {
        queryType = QueryType::INSERT;
        parseInsertQuery();
    } else if (string_util::toLowercase(queryString).find("update") == 0) {
        queryType = QueryType::UPDATE;
        parseUpdateQuery();
    } else if (string_util::toLowercase(queryString).find("delete") == 0) {
        queryType = QueryType::DELETE;
        parseDeleteQuery();
    } else if (string_util::toLowercase(queryString).find("select") == 0) {
        queryType = QueryType::SELECT;
        parseSelectQuery();
    } else {
        throw InvalidQueryException("Invalid query");
    }
}

void Query::parseCreateQuery() {
    QueryParts parts = string_util::split(queryString, ' ');
    // A CREATE query must have at least 8 parts:
    // CREATE TABLE tableName ( colName dataType ) ;
    if (parts.size() < 8) {
        throw InvalidQueryException("Malformed query");
    }
    properties["tableName"] = parts[2];
    if (string_util::toLowercase(parts[1]) != "table" || parts[3] != "("
            || parts[parts.size() - 2] != ")") {
        throw InvalidQueryException("Malformed query");
    }
    Schema schema;
    unsigned int index = 4;
    bool primaryKeyFound = false;
    std::set<std::string> colNames;
    std::vector<ColumnMetadata> metadataVec;
    while (index < parts.size() - 2) {
        if (string_util::toLowercase(parts[index]) == "primary") {
            parsePrimaryKey(parts, metadataVec, index);
        } else {
            ColumnMetadata metadata = createColumnMetadata(parts[2], parts,
                    index);
            metadataVec.push_back(metadata);
        }
    }
    for (const auto& metadata : metadataVec) {
        ensureValidMetadata(metadata, colNames, primaryKeyFound);
        schema.addColumn(metadata);
    }
    properties["schema"] = schema.toString();
}

void Query::parseDropQuery() {
    QueryParts parts = string_util::split(queryString, ' ');
    // A DROP query has 4 parts:
    // DROP TABLE tableName ;
    if (parts.size() != 4) {
        throw InvalidQueryException("Malformed query");
    }
    properties["tableName"] = parts[2];
    if (string_util::toLowercase(parts[1]) != "table") {
        throw InvalidQueryException("Expected 'table' but got " + parts[1]);
    }
}

void Query::parseInsertQuery() {
    QueryParts parts = string_util::split(queryString, ' ', true);
    // An INSERT query has at least 11 parts:
    // INSERT INTO tableName ( colName ) VALUES ( colValue ) ;
    if (parts.size() < 11) {
        if (parts[3] != "(") {
            throw InvalidQueryException("Expected column names after table "
                    "name");
        } else {
            throw InvalidQueryException("Malformed query");
        }
    } else if (string_util::toLowercase(parts[1]) != "into") {
        throw InvalidQueryException("Expected 'into' after insert keyword");
    }
    properties["tableName"] = parts[2];
    populateInsertColumnProperties(properties, parts);
}

void Query::parseUpdateQuery() {
    QueryParts parts = string_util::split(queryString, ' ', true);
    // An UPDATE query has at least 7 parts:
    // UPDATE tableName SET colName = newValue ;
    if (parts.size() < 7 || string_util::toLowercase(parts[2]) != "set") {
        throw InvalidQueryException("Malformed query");
    }
    properties["tableName"] = parts[1];
    properties["columns"] = properties["values"] = "";
    unsigned int index = 3;
    while (parts[index] != ";"
            && string_util::toLowercase(parts[index]) != "where") {
        properties["columns"] += parts[index++] + ",";
        if (parts[index++] != "=") {
            throw InvalidQueryException("Expected = after column name");
        }
        if (string_util::toLowercase(parts[index]) == "null") {
            parts[index] = Column::NULL_VALUE;
        }
        properties["values"] += parts[index++] + ",";
        if (parts[index] == ",") {
            index++;
        }
    }
    // Erase extra commas
    properties["columns"].erase(properties["columns"].length() - 1);
    properties["values"].erase(properties["values"].length() - 1);
    // Parse restrictions
    properties["restrictions"] = parseRestrictions(parts, index);
}

void Query::parseDeleteQuery() {
    QueryParts parts = string_util::split(queryString, ' ', true);
    // A DELETE query has at least 4 parts:
    // DELETE FROM tableName ;
    if (parts.size() < 4 || string_util::toLowercase(parts[1]) != "from") {
        throw InvalidQueryException("Malformed query");
    }
    properties["tableName"] = parts[2];
    // Parse restrictions
    unsigned int index = 3;
    properties["restrictions"] = parseRestrictions(parts, index);
}

void Query::parseSelectQuery() {
    QueryParts parts = string_util::split(queryString, ' ', true);
    // A SELECT query has at least 5 parts:
    // SELECT columnName FROM tableName ;
    // It must also include a FROM clause
    if (parts.size() < 5 || string_util::toLowercase(queryString).find(" from ")
            == std::string::npos) {
        throw InvalidQueryException("Malformed query");
    }
    unsigned int index = 1;
    if (string_util::toLowercase(parts[1]) == "distinct") {
        index = 2;
        properties["distinct"] = "";
    }
    properties["columnNames"] = "";
    while (string_util::toLowercase(parts[index]) != "from") {
        properties["columnNames"] += string_util::extractQuoted(parts[index++]);
    }
    index++;
    properties["tableNames"] = "";
    while (string_util::toLowercase(parts[index]) != "where"
            && string_util::toLowercase(parts[index]) != "order"
            && parts[index] != ";") {
        properties["tableNames"] += string_util::extractQuoted(parts[index++]);
    }
    // Parse restrictions
    properties["restrictions"] = parseRestrictions(parts, index);
    properties["joinConditions"] = extractJoinConditions(properties);
    properties["orderBy"] = parseOrderBy(parts, index);
    if (string_util::toLowercase(parts[index]) == "desc") {
        properties["desc"] = "";
    }
}

void Query::parsePrimaryKey(const QueryParts& parts,
        MetadataVec& metadataVec, unsigned int& index) {
    if (string_util::toLowercase(parts[index + 1]) != "key") {
        throw InvalidQueryException("Expected 'key' after 'primary'");
    }
    if (parts[index + 2] != "(" || parts[index + 4] != ")") {
        throw InvalidQueryException("Expected parentheses after primary"
                " key declaration");
    }
    for (auto& metadata : metadataVec) {
        if (metadata.getColumnName() == parts[index + 3]) {
            metadata.primaryKey = metadata.notNull = true;
        }
    }
    index += (parts[index + 5] == "," ? 6 : 5);
}
