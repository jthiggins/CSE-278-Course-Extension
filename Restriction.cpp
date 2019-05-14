/* 
 * File:   Table.cpp
 * Implementation file for the Restriction class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stack>
#include <string>
#include <vector>
#include <stdexcept>
#include "Restriction.h"
#include "Row.h"
#include "string_util.h"
#include "InvalidQueryException.h"

// Helper functions
namespace {

    /**
     * Compares the values using the given operator.
     * 
     * @return The result of the comparison
     */
    template<typename T>
    bool compareValues(const T& val1, const std::string& op, const T& val2) {
        if (op == "=") {
            return val1 == val2;
        } else if (op == "<") {
            return val1 < val2;
        } else if (op == "<=") {
            return val1 <= val2;
        } else if (op == ">") {
            return val1 > val2;
        } else if (op == ">=") {
            return val1 >= val2;
        } else if (op == "!=") {
            return val1 != val2;
        }
        throw std::invalid_argument("Invalid operator: " + op);
    }

    /**
     * Compares the values as if they were in the condition 'val1 LIKE val2'.
     * @return The result of the comparison
     */
    bool compareLike(const std::string& val1, const std::string& val2) {
        std::string regexStr = string_util::escapeRegex(val2);
        regexStr = string_util::replace(regexStr, "%", ".*");
        regexStr = string_util::replace(regexStr, "_", ".");
        std::regex regex(regexStr);
        return std::regex_match(val1, regex);
    }

    /**
     * Checks that the given types are compatible.
     * 
     * @return True if the types are compatible, false otherwise
     */
    bool compareColumnTypes(const std::string& col1Type,
            const std::string& col2Type) {
        if (col1Type.find("char") != std::string::npos) {
            return col2Type.find("char") != std::string::npos;
        }
        return col1Type == col2Type;
    }

    /**
     * Gets the column value if s names a valid column. Otherwise, returns s.
     * 
     * @param s The (potential) name of the column to search for
     * @param row The row to pull values from
     * @return The value of the column with name s, or s if no such column 
     * exists
     */
    std::string getColumnValue(const std::string& s, const Row& row) {
        if (string_util::toLowercase(s) == "null") {
            return Column::NULL_VALUE;
        }
        try {
            Column c = row.getColumn(s);
            return static_cast<std::string> (c);
        } catch (std::exception& e) {
            if (s.at(0) != '"' && s.at(0) != '\'') {
                try {
                    std::stod(s);
                } catch (std::exception& e) {
                    throw InvalidQueryException("Invalid value/column name: "
                            + s);
                }
            }
        }
        return s;
    }

    /**
     * Evaluates one restriction of the form 'first op second' using the given
     * row to pull column values from if needed.
     * 
     * @return The result of the evaluation
     */
    bool evaluateRestriction(const std::string& first, const std::string& op,
            const std::string& second, const Row& row) {
        Column col1, col2;
        std::string col1Value = getColumnValue(first, row);
        std::string col2Value = getColumnValue(second, row);

        if (col1Value != first && col1Value != Column::NULL_VALUE) {
            col1 = row.getColumn(first);
        }
        if (col2Value != second && col2Value != Column::NULL_VALUE) {
            col2 = row.getColumn(second);
        }

        bool compareNumeric = false, compareFloatingPoint = false,
                compareDates = false, compareTimes = false;
        if (col1 || col2) {
            std::string col1Type, col2Type;
            col1Type = (col1 ? col1.getMetadata().getColumnType() :
                    col2 ? col2.getMetadata().getColumnType() : "");
            col2Type = (col2 ? col2.getMetadata().getColumnType() :
                    col1 ? col1.getMetadata().getColumnType() : "");
            if (!compareColumnTypes(col1Type, col2Type)) {
                throw std::invalid_argument(first + " and " + second + " do not"
                        " have the same types");
            }
            if (col1Type == "int" || col1Type == "bigint") {
                compareNumeric = true;
            } else if (col1Type == "float" || col1Type == "double") {
                compareFloatingPoint = true;
            } else if (col1Type == "date") {
                compareDates = true;
            } else if (col1Type == "time") {
                compareTimes = true;
            }
        }
        if (compareNumeric && col1Value != "" && col2Value != "") {
            return compareValues<long long>(std::stoll(col1Value), op,
                    std::stoll(col2Value));
        } else if (compareFloatingPoint && col1Value != "" && col2Value != "") {
            return compareValues<double>(std::stod(col1Value), op,
                    std::stod(col2Value));
        } else if (compareDates && col1Value != "" && col2Value != "") {
            return compareValues<boost::gregorian::date>(
                    boost::gregorian::from_string(col1Value), op,
                    boost::gregorian::from_string(col2Value));
        } else if (compareTimes && col1Value != "" && col2Value != "") {
            return compareValues<boost::posix_time::ptime>(
                    boost::posix_time::time_from_string(col1Value), op,
                    boost::posix_time::time_from_string(col2Value));
        } else {
            if (string_util::toLowercase(op) != "like") {
                return compareValues<std::string>(
                        string_util::extractQuoted(col1Value), op,
                        string_util::extractQuoted(col2Value));
            } else {
                return compareLike(string_util::extractQuoted(col1Value),
                        string_util::extractQuoted(col2Value));
            }
        }
    }
}  // namespace

Restriction::Restriction(const std::string& restriction)
: restriction(restriction) {
    parseRestriction();
}

Restriction::~Restriction() {
    // No implementation needed
}

bool Restriction::apply(const Row& row) {
    if (restriction == "") {
        return true;
    }
    // Assumes restrictions have been parsed into postfix expression
    std::stack<bool> restrictionResultStack;
    auto parts = string_util::split(restriction, ' ', true);
    for (unsigned int i = 0; i < parts.size();) {
        if (parts[i] != "and" && parts[i] != "or") {
            bool result = evaluateRestriction(parts[i], parts[i + 1],
                    parts[i + 2], row);
            restrictionResultStack.push(result);
            i += 3;
        }
        if (i < parts.size() && (parts[i] == "and" || parts[i] == "or")) {
            bool res1 = restrictionResultStack.top();
            restrictionResultStack.pop();
            bool res2 = restrictionResultStack.top();
            restrictionResultStack.pop();
            restrictionResultStack.push((parts[i] == "and") ? res1 && res2 :
                    res1 || res2);
            i++;
        }
    }
    return restrictionResultStack.top();
}

bool Restriction::isEmpty() {
    return restriction.empty();
}


void Restriction::parseRestriction() {
    if (restriction == "") {
        return;
    }
    std::stack<std::string> operatorStack;
    auto parts = string_util::split(restriction, ' ', true);
    std::ostringstream restrictionStream;
    for (const auto& part : parts) {
        if (string_util::toLowercase(part) == "and"
                || string_util::toLowercase(part) == "or" || part == "(") {
            operatorStack.push(string_util::toLowercase(part));
        } else if (part == ")") {
            std::string op;
            while ((op = operatorStack.top()) != "(") {
                operatorStack.pop();
                restrictionStream << op << " ";
            }
            operatorStack.pop();
        } else {
            restrictionStream << part << " ";
        }
    }
    while (!operatorStack.empty()) {
        restrictionStream << operatorStack.top() << " ";
        operatorStack.pop();
    }
    restriction = restrictionStream.str();
    // Erase last space
    restriction.erase(restriction.length() - 1);
}

