/* 
 * File:   InvalidQueryException.h
 * Header file for the InvalidQueryException class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef INVALIDQUERYEXCEPTION_H
#define INVALIDQUERYEXCEPTION_H

#include <exception>
#include <string>

class InvalidQueryException : public std::exception {
public:
    InvalidQueryException(const std::string& message) : message(message) {}
    ~InvalidQueryException();
    const char* what() const throw() override;
    
private:
    std::string message;
};

#endif /* INVALIDQUERYEXCEPTION_H */

