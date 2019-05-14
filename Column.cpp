/* 
 * File:   Column.cpp
 * Implementation file for the Column class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#include <iomanip>
#include <iostream>
#include <string>
#include <typeinfo>
#include "Column.h"

const std::string Column::UNDEFINED = "\x7F";
const std::string Column::NULL_VALUE = "\0";

std::ostream& operator<<(std::ostream& os, const Column& col) {
    os << std::quoted(col.colValue);
    return os;
}

// Constructors implemented in header

Column::~Column() {
    // No implementation needed
}

ColumnMetadata Column::getMetadata() const {
    return metadata;
}

bool Column::isNull() const {
    return colValue == NULL_VALUE;
}

Column::operator int() const {
    size_t pos = 0;
    int result = std::stoi(colValue, &pos);
    if (pos != colValue.length()) {
        throw std::bad_cast();
    }
    return result;
}

Column::operator long long() const {
    size_t pos = 0;
    long result = std::stoll(colValue, &pos);
    if (pos != colValue.length()) {
        throw std::bad_cast();
    }
    return result;
}

Column::operator double() const {
    return std::stod(colValue);
}

Column::operator float() const {
    return std::stof(colValue);
}

Column::operator std::string() const {
    return colValue;
}

Column::operator boost::gregorian::date() const {
    return boost::gregorian::from_string(colValue);
}

Column::operator boost::posix_time::ptime() const {
    boost::gregorian::date d(boost::gregorian::day_clock::local_day());
    auto dateStr = boost::gregorian::to_iso_extended_string(d);
    return boost::posix_time::time_from_string(dateStr + " " + colValue);
}

Column::operator bool() const {
    return colValue != UNDEFINED;
}

bool Column::operator==(const Column& col) {
    std::string dataType = metadata.getColumnType();
    if (dataType.find("char") != std::string::npos) {
        return colValue == col.colValue;
    } else if (dataType == "int" || dataType == "bigint") {
        return static_cast<long long>(*this) == static_cast<long long>(col);
    } else if (dataType == "float" || dataType == "double") {
        return static_cast<double>(*this) == static_cast<double>(col);
    } else if (dataType == "date") {
        return this->operator boost::gregorian::date()
                == col.operator boost::gregorian::date();
    } else if (dataType == "time") {
        return this->operator boost::posix_time::ptime()
                == col.operator boost::posix_time::ptime();
    }
    return false;
}

bool Column::operator<(const Column& col) {
    std::string dataType = metadata.getColumnType();
    if (dataType.find("char") != std::string::npos) {
        return colValue < col.colValue;
    } else if (dataType == "int" || dataType == "bigint") {
        return static_cast<long long>(*this) < static_cast<long long>(col);
    } else if (dataType == "float" || dataType == "double") {
        return static_cast<double>(*this) < static_cast<double>(col);
    } else if (dataType == "date") {
        return this->operator boost::gregorian::date() 
                < col.operator boost::gregorian::date();
    } else if (dataType == "time") {
        return this->operator boost::posix_time::ptime() 
                < col.operator boost::posix_time::ptime();
    }
    return false;
}

bool Column::operator>(const Column& col) {
    std::string dataType = metadata.getColumnType();
    if (dataType.find("char") != std::string::npos) {
        return colValue > col.colValue;
    } else if (dataType == "int" || dataType == "bigint") {
        return static_cast<long long>(*this) > static_cast<long long>(col);
    } else if (dataType == "float" || dataType == "double") {
        return static_cast<double>(*this) > static_cast<double>(col);
    } else if (dataType == "date") {
        return this->operator boost::gregorian::date() 
                > col.operator boost::gregorian::date();
    } else if (dataType == "time") {
        return this->operator boost::posix_time::ptime() 
                > col.operator boost::posix_time::ptime();
    }
    return false;
}

bool Column::operator<=(const Column& col) {
    std::string dataType = metadata.getColumnType();
    if (dataType.find("char") != std::string::npos) {
        return colValue <= col.colValue;
    } else if (dataType == "int" || dataType == "bigint") {
        return static_cast<long long>(*this) <= static_cast<long long>(col);
    } else if (dataType == "float" || dataType == "double") {
        return static_cast<double>(*this) <= static_cast<double>(col);
    } else if (dataType == "date") {
        return this->operator boost::gregorian::date() 
                <= col.operator boost::gregorian::date();
    } else if (dataType == "time") {
        return this->operator boost::posix_time::ptime() 
                <= col.operator boost::posix_time::ptime();
    }
    return false;
}

bool Column::operator>=(const Column& col) {
    std::string dataType = metadata.getColumnType();
    if (dataType.find("char") != std::string::npos) {
        return colValue >= col.colValue;
    } else if (dataType == "int" || dataType == "bigint") {
        return static_cast<long long>(*this) >= static_cast<long long>(col);
    } else if (dataType == "float" || dataType == "double") {
        return static_cast<double>(*this) >= static_cast<double>(col);
    } else if (dataType == "date") {
        return this->operator boost::gregorian::date() 
                >= col.operator boost::gregorian::date();
    } else if (dataType == "time") {
        return this->operator boost::posix_time::ptime() 
                >= col.operator boost::posix_time::ptime();
    }
    return false;
}
