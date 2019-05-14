/* 
 * File:   Restriction.h
 * Header file for the Restriction class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef RESTRICTION_H
#define RESTRICTION_H

#include <stack>
#include <string>
#include "Row.h"

/**
 * Represents a restriction on a row. Used for writing to and reading from 
 * tables.
 */
class Restriction {
public:
    Restriction(const std::string& restriction);
    ~Restriction();
    
    /**
     * Tests whether or not the given row matches the restriction.
     * 
     * @param row The row to test
     * @return True if the row matches the restriction, false otherwise
     */
    bool apply(const Row& row);
    
    /**
     * Checks if the restriction is empty (has a value of "").
     */
    bool isEmpty();
    
private:
    std::string restriction;
    
    /**
     * Transforms the restriction into a postfix expression. A postfix 
     * expression is one where the operator comes after both operands. For 
     * example, the expression "restriction1 restriction2 and" would mean
     * "restriction1 and restriction2". This makes the result of the restriction
     * applied to a row much easier to compute.
     */
    void parseRestriction();
};

#endif /* RESTRICTION_H */

