/* 
 * File:   Column.h
 * Header file for the Column class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef COLUMN_H
#define COLUMN_H

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <string>

#include "ColumnMetadata.h"

/**
 * Represents a column in a row. Columns are automatically type casted into
 * supported data types.
 */
class Column {
public:    
    friend std::ostream& operator<<(std::ostream& os, const Column& col);
    /** The value used to indicate null columns. */
    static const std::string NULL_VALUE;
    
    Column() : colValue(UNDEFINED) {}
    Column(const std::string& colValue) : colValue(colValue) {}
    Column(const std::string& colValue, const ColumnMetadata& metadata) 
             : colValue(colValue), metadata(metadata) {}
    ~Column();
    
    /** Gets the metadata for this column. */
    ColumnMetadata getMetadata() const;
    
    /** Checks to see if the column holds a null value. */
    bool isNull() const;
    
    // Type conversions
    operator int() const;
    operator long long() const;
    operator float() const;
    operator double() const;
    operator std::string() const;
    operator boost::gregorian::date() const;
    operator boost::posix_time::ptime() const;
    // Used to tell if the column has been initialized
    operator bool() const;
    
    // Comparison operators
    bool operator==(const Column& col);
    bool operator<(const Column& col);
    bool operator>(const Column& col);
    bool operator<=(const Column& col);
    bool operator>=(const Column& col);
    
private:
    std::string colValue;
    ColumnMetadata metadata;
    /** Value used for uninitialized columns */
    static const std::string UNDEFINED;
};

#endif /* COLUMN_H */

