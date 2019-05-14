/* 
 * File:   JoinedTable.h
 * Header file for the JoinedTable class.
 *
 * Copyright (C) Jared Higgins (higginjt@miamioh.edu) 2019
 */

#ifndef JOINEDTABLE_H
#define JOINEDTABLE_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Row.h"
#include "Table.h"

using ColNames = std::vector<std::string>;
using ProbeToBuildMap = std::unordered_map<std::string, std::string>;
using UpdateMap = std::unordered_map<std::string, std::string>;
using JoinMap = std::unordered_map<std::string, Row>;

/**
 * Represents a table that is the result of joining two or more tables. This 
 * table does not support modification of underlying data; calls to any modifier
 * methods inherited from Table will throw an error.
 */
class JoinedTable : public Table {
public:
    JoinedTable(const Table& table1, const Table& table2,
            const std::string& joinCondition);
    virtual ~JoinedTable() override;
    
    virtual JoinedTable& operator>>(Row& row) override;
    
    virtual void insertRow(Row& row) override;

    virtual void updateRows(UpdateMap& columnsToUpdate) override;
    
    virtual void deleteRows() override;
    
    virtual Table& orderBy(const std::string& colNames, bool desc) override;
    
    virtual operator bool() const override;
    
    unsigned int getRowCount() const override;
    
    virtual std::shared_ptr<Table> clone() const override;

    
private:
    std::shared_ptr<Table> buildTable, probeTable;
    ProbeToBuildMap columnMap;
    JoinMap joinMap;
    
    /**
     * Builds the join map used in the hash join algorithm for joining tables.
     * @param colNames The names of the columns in the build table to store
     * in the map
     */
    void buildJoinMap(const ColNames& colNames);
    
    /** 
     * Assigns the build and probe tables according to the tables being 
     * joined.
     */
    void assignBuildAndProbeTables(const Table& table1, const Table& table2);
    
    /**
     * Parses the join conditions and builds the join map.
     * 
     * @param parts The parts of the join condition passed in to the constructor
     */
    void parseJoinCondition(const std::vector<std::string>& parts);
    
    /** Extracts the next row from the table. */
    void extractRow(Row& row);
    
    /** 
     * Extracts the next row according to the join conditions provided
     * to the table.
     */
    void extractRowJoined(Row& row);
};

#endif /* RESTRICTEDTABLE_H */

