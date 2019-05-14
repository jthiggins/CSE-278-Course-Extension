/* 
 * File:   string_util.cpp
 * Implementation file for string_util.h.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "string_util.h"

// Helper functions
namespace {
    /**
     * Gets the escaped version of the given character. Does not support
     * octal, hexadecimal, or Unicode escapes.
     * 
     * @param c The character to escape
     * @return The escaped character
     * @throw std::invalid_argument if the character cannot be escaped
     */
    char getEscapedChar(char c) {
        switch (c) {
            case '\'':
                return '\'';
            case '"':
                return '\"';
            case '\\':
                return '\\';
            default:
                throw std::invalid_argument("Cannot escape character " + c);
        }
    }
}  // namespace

std::string string_util::toLowercase(const std::string& s) {
    std::ostringstream retStream;
    for (const auto& c : s) {
        retStream << static_cast<char>(std::tolower(c));
    }
    return retStream.str();
}

std::vector<std::string> string_util::split(const std::string& s, 
        const char delim, bool ignoreQuotes) {
    std::vector<std::string> ret;
    std::ostringstream os;
    bool escaped = false, quoted = false;
    char quoteChar = '\0';
    for (const auto& c : s) {
        if (ignoreQuotes) {
            if (c == '\\') {
                escaped = !escaped;
            } else {
                if ((c == '"' || c == '\'') && !escaped) {
                    if (c == quoteChar || quoteChar == '\0') {
                        quoted = !quoted;
                        quoteChar = quoted ? c : '\0';
                    }
                } 
                escaped = false;
            } 
        }
        if (c == delim && (!quoted || (!escaped && (c == '\'' || c == '"')))) {
            ret.push_back(os.str());
            // Clear contents of os
            os.str("");
        } else {
            os << c;
        }
    }
    ret.push_back(os.str());
    return ret;
}

std::string string_util::extractQuoted(const std::string& s) {
    if (s.empty()) {
        return s;
    }
    char quoteChar = s.at(0);
    if ((quoteChar != '"' && quoteChar != '\'') 
            || s.find_last_of(quoteChar) != s.length() - 1) {
        return s;
    }
    bool escaped = false;
    for (unsigned int i = 1; i < s.length() - 1; i++) {
        if (s.at(i) == '\\') {
            escaped = !escaped;
        } else {
            if (s.at(i) == quoteChar && !escaped) {
                return s;
            } 
            escaped = false;
        } 
    }
    return s.substr(1, s.length() - 2);
}

std::string string_util::getEscapedString(const std::string& s) {
    std::ostringstream out;
    bool escaped = false;
    for (const auto& c : s) {
        if (c == '\\' && !escaped) {
            escaped = true;
        } else {
            out << (escaped ? getEscapedChar(c) : c);
            escaped = false;
        }
    }
    return out.str();
}

std::string string_util::replace(const std::string& s, 
        const std::string& toReplace, const std::string& repValue) {
    std::ostringstream retStream;
    auto start = 0;
    auto stop = s.find(toReplace);
    while (stop != std::string::npos) {
        retStream << s.substr(start, (stop - start)) << repValue;
        start = stop + toReplace.length();
        stop = s.find(toReplace, start);
    }
    retStream << s.substr(start);
    return retStream.str();
}

std::string string_util::escapeRegex(const std::string& s) {
    std::ostringstream retStream;
    for (const auto& c : s) {
        switch (c) {
            case '[':
            case '\\':
            case '^':
            case '$':
            case '.':
            case '|':
            case '?':
            case '*':
            case '+':
            case '(':
            case ')':
            case '{':
            case '}':
                retStream << '\\' << c;
                break;
            default:
                retStream << c;
                break;
        }
    }
    return retStream.str();
}
