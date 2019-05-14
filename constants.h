/* 
 * File:   constants.h
 * Constant definitions used in the database management system.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

/** The directory in which tables are stored */
const std::string TABLE_DIRECTORY = "./tables/";
/** The extension used for table files */
const std::string TABLE_EXTENSION = ".table";
/** The extension used for temporary files created when modifying tables */
const std::string TEMP_EXTENSION = ".tmp";

#endif /* CONSTANTS_H */

