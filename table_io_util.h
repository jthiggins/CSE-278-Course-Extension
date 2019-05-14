/* 
 * File:   table_io_util.h
 * A collection of utility functions for table I/O.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef TABLE_IO_UTIL_H
#define TABLE_IO_UTIL_H

#include <experimental/filesystem>
#include <string>
#include "ColumnMetadata.h"

namespace fs = std::experimental::filesystem;

namespace table_io_util {

    /**
     * Formats the given column value to be consistent with the given type.
     * This includes removing quotes and escaping characters when appropriate.
     * 
     * @param colType The type of the given value
     * @param colValue The value to format
     */
    void formatColumnValue(const std::string& colType, std::string& colValue);

    /**
     * Ensures that the value being modified will reference an existing value 
     * for the column referenced by this column after modification.
     * 
     * @param metadata The metadata for the column to check
     * @param colValue The value of the column
     */
    void validateReferencedColumn(const ColumnMetadata& metadata,
            const std::string& colValue);

    /**
     * See validateReferencedBy(ColumnMetadata, std::string).
     * 
     * @param path The path of the table being searched for references
     */
    void validateReferencedBy(const ColumnMetadata& metadata,
            const std::string& oldValue, const fs::path& path);

    /**
     * Ensures that the column value being modified is not referenced by any
     * other column in the database.
     * 
     * @param metadata The metadata for the column to check
     * @param oldValue The value of the column before modification
     */
    void validateReferencedBy(const ColumnMetadata& metadata,
            const std::string& oldValue);
}  // namespace table_io_util

#endif /* TABLE_IO_UTIL_H */

