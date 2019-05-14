/* 
 * File:   InvalidQueryException.cpp
 * Implementation file for the InvalidQueryException class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */
#include <string>
#include "InvalidQueryException.h"

InvalidQueryException::~InvalidQueryException() {
    // No implementation needed
}

const char* InvalidQueryException::what() const throw() {
    return message.c_str();
}

