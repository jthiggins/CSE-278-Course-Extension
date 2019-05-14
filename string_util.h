/* 
 * File:   string_util.h
 * A collection of utility functions for string operations.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef UTILITY_H
#define UTILITY_H

#include <string>
#include <vector>

namespace string_util {
    /**
     * Converts the given string to lowercase.
     * 
     * @param s The string to convert
     * @return The converted string
     */
    std::string toLowercase(const std::string& s);

    /**
     * Splits a string on a delimiter.
     * 
     * @param s The string to split.
     * @param delim The delimiter to split on.
     * @param ignoreQuotes Whether or not characters that are quoted should
     * be considered valid delimeters
     * @return A vector containing the parts of the given string separated by 
     * the delimiter
     */
    std::vector<std::string> split(const std::string& s,
            const char delim, bool ignoreQuotes = false);
    
    /** 
     * Extracts the inside of a quoted string. Equivalent to using 
     * s.substr(1, s.length() - 2), except substr() does not check to see if
     * s is properly quoted (i.e., all inner quote characters are escaped).
     * 
     * @param s The quoted string
     * @return The contents of the quoted string, or the orignal string if it
     * is not properly quoted.
     */
    std::string extractQuoted(const std::string& s);
    
    /**
     * Returns the given string with escaped characters properly replaced.
     * 
     * @param s The string to escape
     * @return The escaped string
     */
    std::string getEscapedString(const std::string& s);
    
    /**
     * Replaces all occurrences of a substring inside a string 
     * with another string. Does not modify the input string.
     * 
     * @param s The string to modify
     * @param toReplace The substring to replace
     * @param repValue The string to replace the substring with
     * @return s with all instances of toReplace replaced by repValue
     */
    std::string replace(const std::string& s, const std::string& toReplace,
            const std::string& repValue);
    
    /**
     * Escapes all regex characters in the given string.
     * 
     * @param s The string to escape
     * @return The escaped string
     */
    std::string escapeRegex(const std::string& s);
}  // namespace string_util

#endif /* UTILITY_H */

