/* 
 * File:   Result.cpp
 * Implementation file for the Result class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */
#include <stdio.h>
#include <boost/asio.hpp>
#include <experimental/filesystem>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "constants.h"
#include "InvalidQueryException.h"
#include "Query.h"
#include "Result.h"
#include "Schema.h"
#include "string_util.h"
#include "table_io_util.h"
#include "JoinedTable.h"

using QueryParts = std::vector<std::string>;
using ColumnInfo = std::vector<std::string>;

// Helper functions
namespace {

    /**
     * Ensures that the given referenced column is valid.
     * 
     * @param tableName The name of the table to search
     * @param colName The name of the column to look for
     * @param dataType The expected data type
     * @throw InvalidQueryException if the column is not valid
     */
    void checkValidReferencedColumn(const std::string& tableName,
            const std::string& colName, const std::string& dataType) {
        std::ifstream is(TABLE_DIRECTORY + tableName + TABLE_EXTENSION);
        if (!is.good())
            throw InvalidQueryException("Table " + tableName + " not found");
        std::string schemaString;
        std::getline(is, schemaString);
        Schema schema(tableName, schemaString);
        if (!schema.hasColumn(colName)) {
            throw InvalidQueryException("Column " + colName + " not found in "
                    "table " + tableName);
        }
        if (schema.getColumnMetadata(colName).getColumnType() != dataType) {
            throw InvalidQueryException("Column " + colName + " in table "
                    + tableName + " does not have data type " + dataType);
        }
    }

    /**
     * Ensures that the referenced columns in the given schema are valid.
     * 
     * @param schema The schema to check
     * @throw InvalidQueryException if one or more columns are invalid
     */
    void checkReferencedColumns(const Schema& schema) {
        for (const auto& metadata : schema.getMetadataForColumns()) {
            std::string referencedCol = metadata.getReferencedColumn();
            if (referencedCol != "") {
                if (referencedCol.find(".") == std::string::npos) {
                    if (!schema.hasColumn(referencedCol)) {
                        throw InvalidQueryException("Column " + referencedCol
                                + " does not exist");
                    } else if (schema.getColumnMetadata(referencedCol)
                            .getColumnType() != metadata.getColumnType()) {
                        throw InvalidQueryException("Column " + referencedCol
                                + " does not have data type "
                                + metadata.getColumnType());
                    }
                } else if (referencedCol.find(".") != std::string::npos) {
                    std::vector<std::string> parts =
                            string_util::split(referencedCol, '.');
                    checkValidReferencedColumn(parts[0], parts[1],
                            metadata.getColumnType());
                }
            }
        }
    }

    /**
     * Gets the path to the file for the given table.
     * 
     * @param tableName The name of the table
     * @return The path to the table file
     */
    std::string getPathToTableFile(const std::string& tableName) {
        return TABLE_DIRECTORY + tableName + TABLE_EXTENSION;
    }

    /**
     * Inserts a new entry into the given table.
     * 
     * @param tableFile The file where the table is stored
     * @param schema The schema of the table
     * @param colNames The names of the columns as retrieved from the query
     * @param colValues The values of the columns as retrieved from the query
     */
    void insertIntoTable(std::string& tableName, const Schema& schema,
            const ColumnInfo& colNames, const ColumnInfo& colValues) {
        std::vector<std::string> orderedColValues;
        unsigned int index = 0;
        for (const auto& metadata : schema.getMetadataForColumns()) {
            auto colName = metadata.getColumnName();
            unsigned int colIndex = std::distance(colNames.begin(),
                    std::find(colNames.begin(), colNames.end(), colName));
            if (colIndex >= colNames.size()) {
                throw InvalidQueryException("Column not specified: " + colName);
            }
            orderedColValues.push_back(colValues[colIndex]);
            index++;
        }
        Table table(tableName, schema);
        Row row = Row(schema, orderedColValues);
        table.insertRow(row);
    }

    /**
     * Executes a CREATE query.
     * 
     * @param query The query to execute.
     */
    void executeCreateQuery(const Query& query) {
        std::string tableName = query.getProperty("tableName");
        std::string tablePath = getPathToTableFile(tableName);
        // Check referenced columns
        Schema schema(tableName, query.getProperty("schema"));
        checkReferencedColumns(schema);
        if (std::experimental::filesystem::exists(tablePath))
            throw InvalidQueryException(tableName + " already exists");
        std::experimental::filesystem::create_directory(TABLE_DIRECTORY);
        std::ofstream out(tablePath);
        out << query.getProperty("schema") << std::endl;
    }

    /**
     * Executes a DROP query.
     * 
     * @param query The query to execute.
     */
    void executeDropQuery(const Query& query) {
        std::string tableName = query.getProperty("tableName");
        std::string tablePath = getPathToTableFile(tableName);
        if (!std::experimental::filesystem::exists(tablePath))
            throw InvalidQueryException(tableName + " does not exist");
        std::fstream tableFile(TABLE_DIRECTORY + tableName + TABLE_EXTENSION);
        std::string schemaStr;
        std::getline(tableFile, schemaStr);
        Row row(Schema(tableName, schemaStr));
        while (tableFile >> row) {
            for (const auto& col : row.getColumns()) {
                table_io_util::validateReferencedBy(col.getMetadata(), col);
            }
        }
        std::remove(tablePath.c_str());
    }

    /**
     * Executes an INSERT query.
     * 
     * @param query The query to execute
     */
    void executeInsertQuery(const Query& query) {
        std::string tableName = query.getProperty("tableName");
        std::string tablePath = getPathToTableFile(tableName);
        if (!std::experimental::filesystem::exists(tablePath))
            throw InvalidQueryException(tableName + " does not exist");
        std::fstream tableFile(tablePath, std::ios::in | std::ios::app);
        std::string schemaString;
        std::getline(tableFile, schemaString);
        Schema schema(tableName, schemaString);
        QueryParts colNames =
                string_util::split(query.getProperty("columnNames"), ',');
        QueryParts colValues =
                string_util::split(query.getProperty("columnValues"), ',',
                true);
        if (colNames.size() != colValues.size()) {
            throw InvalidQueryException("Number of columns and values must "
                    "match");
        }
        for (const auto& colName : colNames) {
            if (!schema.hasColumn(colName)) {
                throw InvalidQueryException("Unknown column: " + colName);
            }
        }
        insertIntoTable(tableName, schema, colNames, colValues);
    }

    /**
     * Executes an UPDATE query.
     * 
     * @param query The query to execute
     */
    void executeUpdateQuery(const Query& query) {
        std::string tableName = query.getProperty("tableName");
        std::string tablePath = TABLE_DIRECTORY + tableName + TABLE_EXTENSION;
        if (!std::experimental::filesystem::exists(tablePath)) {
            throw InvalidQueryException(tableName + " does not exist");
        }
        ColumnInfo colNames =
                string_util::split(query.getProperty("columns"), ',', true);
        ColumnInfo colValues =
                string_util::split(query.getProperty("values"), ',', true);
        std::unordered_map<std::string, std::string> nameValueMap;
        for (unsigned int i = 0; i < colNames.size(); i++) {
            nameValueMap[colNames[i]] = colValues[i];
        }
        std::ifstream in(tablePath);
        std::string schemaStr;
        std::getline(in, schemaStr);
        Schema schema(tableName, schemaStr);
        Table table(tableName, schema);
        if (query.getProperty("restrictions") == "") {
            table.updateRows(nameValueMap);
        } else {
            table.setRestrictions(query.getProperty("restrictions"))
                    .updateRows(nameValueMap);
        }
    }

    /**
     * Executes a DELETE query.
     * 
     * @param query The query to execute
     */
    void executeDeleteQuery(const Query& query) {
        std::string tableName = query.getProperty("tableName");
        std::string tablePath = TABLE_DIRECTORY + tableName + TABLE_EXTENSION;
        if (!std::experimental::filesystem::exists(tablePath)) {
            throw InvalidQueryException(tableName + " does not exist");
        }
        std::ifstream in(tablePath);
        std::string schemaStr;
        std::getline(in, schemaStr);
        Schema schema(tableName, schemaStr);
        Table table(tableName, schema);
        if (query.getProperty("restrictions") == "") {
            table.deleteRows();
        } else {
            table.setRestrictions(query.getProperty("restrictions"))
                    .deleteRows();
        }
    }

    /**
     * Retrieves a table from the given URL and stores it in the given
     * shared_ptr.
     * 
     * @param query The query being executed
     * @param url The URL to retrieve that table from
     * @param table The shared_ptr to store the table in
     */
    void extractTableFromURL(const Query& query, const std::string& url,
            std::shared_ptr<Table>& table) {
        using namespace boost::asio::ip;
        auto stopIndex = url.find('/', 7);
        std::string host = url.substr(7, stopIndex - 7);
        std::string resource = url.substr(stopIndex);
        auto stream = std::make_shared<tcp::iostream>(host, "80");
        if (!stream->good()) {
            std::cerr << "Error: Could not connect to given URL" << std::endl;
            return;
        }
        *stream << "GET " << resource << " HTTP/1.1\r\n"
                << "Host: " << host << "\r\nConnection: Close\r\n\r\n";
        std::ostringstream schemaStr;
        std::string line, colName;
        std::getline(*stream, line);
        if (line.find("200 OK") == std::string::npos) {
            std::cerr << "Error accessing file" << std::endl;
            return;
        }
        while (line != "\r") {
            std::getline(*stream, line);
        }
        std::getline(*stream, line);
        std::istringstream schemaScanner(line);
        while (schemaScanner >> colName) {
            schemaStr << std::quoted(colName) << " \"varchar(25)\" \"\" "
                    << "false false\t";
        }
        if (!table) {
            table = std::make_shared<Table>(stream, url,
                    Schema(url, schemaStr.str()));
        } else {
            Table tableToJoin(stream, url, Schema(url, schemaStr.str()));
            auto joinedTable = table->joinTo(tableToJoin,
                    query.getProperty("joinConditions"));
            table = std::make_shared<JoinedTable>(joinedTable);
        }
    }

    /**
     * Executes a SELECT query.
     * 
     * @param query The query to execute
     */
    void executeSelectQuery(const Query& query, std::shared_ptr<Table>& table) {
        auto tableNames = string_util::split(query.getProperty("tableNames"),
                ',');
        for (const auto& tableName : tableNames) {
            if (tableName.find("http://") == 0) {
                extractTableFromURL(query, tableName, table);
                if (!table) {
                    return;
                }
            } else {
                std::string tablePath = TABLE_DIRECTORY + tableName
                        + TABLE_EXTENSION;
                if (!std::experimental::filesystem::exists(tablePath)) {
                    throw InvalidQueryException(tableName + " does not exist");
                }
                std::ifstream in(tablePath);
                std::string schemaStr;
                std::getline(in, schemaStr);
                Schema schema(tableName, schemaStr);
                if (!table) {
                    table = std::make_shared<Table>(tableName, schema);
                } else {
                    Table tableToJoin(tableName, schema);
                    auto joinedTable = table->joinTo(tableToJoin,
                            query.getProperty("joinConditions"));
                    table = std::make_shared<JoinedTable>(joinedTable);
                }
            }
        }
        if (query.getProperty("restrictions") != "") {
            table->setRestrictions(query.getProperty("restrictions"));
        }
        table->orderBy(query.getProperty("orderBy"),
                query.hasProperty("desc"))
                .filterDistinct(query.hasProperty("distinct"))
                .filterColumnsByName(query.getProperty("columnNames"));
    }
}  // namespace

Result::Result(const Query& query) : query(query) {
    executeQuery();
}

Result::~Result() {
    // No implementation needed
}

Result& Result::operator>>(Row& row) {
    if (table) {
        (*table) >> row;
    }
    return *this;
}

Result::operator bool() {
    return table && *table;
}

void Result::executeQuery() {
    if (query.getType() == Query::QueryType::CREATE) {
        executeCreateQuery(query);
    } else if (query.getType() == Query::QueryType::DROP) {
        executeDropQuery(query);
    } else if (query.getType() == Query::QueryType::INSERT) {
        executeInsertQuery(query);
    } else if (query.getType() == Query::QueryType::UPDATE) {
        executeUpdateQuery(query);
    } else if (query.getType() == Query::QueryType::DELETE) {
        executeDeleteQuery(query);
    } else if (query.getType() == Query::QueryType::SELECT) {
        executeSelectQuery(query, table);
    } else {
        throw std::invalid_argument("Invalid query type");
    }
}
