/* 
 * File:   Result.h
 * Header file for the Result class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef RESULT_H
#define RESULT_H

#include "Table.h"

class Query;  // Forward declaration of class Query

/**
 * Represents the result of a query. This class can be used to extract rows
 * returned by the query.
 */
class Result {
public:
    Result(const Query& query);
    Result(const Result& result);
    ~Result();
    
    // Extraction operator
    Result& operator>>(Row& row);
    
    operator bool();
    
private:
    bool hasRestrictedTable = false;
    const Query& query;  // Needs to be a reference due to circular dependencies
    std::shared_ptr<Table> table;
    
    /**
     * Executes the query passed into the constructor.
     */
    void executeQuery();
};

#endif /* RESULT_H */

