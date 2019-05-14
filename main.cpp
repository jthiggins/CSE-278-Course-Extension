/* 
 * File:   main.cpp
 *
 * Created on April 5, 2019, 10:04 PM
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#include <exception>
#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_map>

#include "Query.h"
#include "Result.h"
#include "Row.h"
#include "Table.h"

// Helper functions
namespace {

    const std::unordered_map<std::string, unsigned int> widthForType = {
        {"int", 11},     // Longest int is 11 chars: -2147483648
        {"bigint", 20},  // Longest bigint is 20 chars: -9223372036854775808
        {"float", 15},   // No hard cutoff, but 15 chars should be enough for
                         // most use cases
        {"double", 15},  // Same with double
        {"date", 10},    // 10 chars: YYYY-MM-DD
        {"time", 8}      // 8 chars: hh:mm:ss
    };

    /** Gets the width to display for the given column type. */
    unsigned int getWidthForColumn(const std::string& columnType) {
        if (columnType.find("char") != std::string::npos) {
            auto typeParts = string_util::split(columnType, '(');
            return std::stoi(typeParts[1].substr(0,
                    typeParts[1].size() - 1));
        }
        return widthForType.at(columnType);
    }

    /**
     * Prints the column headers of the given row.
     */
    void printColumnHeaders(const Row& row) {
        for (Column col : row.getColumns()) {
            auto width = getWidthForColumn(col.getMetadata().getColumnType());
            auto header = col.getMetadata().getTableName() + "." 
                          + col.getMetadata().getColumnName();
            std::cout << std::setw(width) << std::left 
                    << header << "  ";
        }
        std::cout << std::endl;
    }

    /**
     * Prints the row, formatted to account for width of columns
     */
    void printRow(const Row& row) {
        for (Column col : row.getColumns()) {
                auto width = getWidthForColumn(col.getMetadata()
                    .getColumnType());
                std::cout << std::setw(width) << std::left 
                          << (col.isNull() ? "NULL" : 
                                static_cast<std::string>(col)) << "  ";
        }
        std::cout << std::endl;
    }

}  // namespace

/**
 * The main function of the program. Handles the CLI.
 */
int main(int argc, char** argv) {
    std::string queryString;
    std::cout << "query> ";
    while (std::getline(std::cin, queryString) && queryString != "quit") {
        try {
            Query query(queryString);
            Result result = query.execute();
            Row row;
            bool firstLine = true;
            while (result >> row) {
                if (firstLine) {
                    std::cout << std::endl;
                    printColumnHeaders(row);
                    std::cout << std::endl;
                    firstLine = false;
                }
                printRow(row);
                std::cout << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
        std::cout << "query> ";
    }
    return 0;
}

