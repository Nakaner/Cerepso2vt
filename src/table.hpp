/*
 * table.hpp
 *
 *  Created on: 12.10.2016
 *      Author: michael
 */

#ifndef SRC_TABLE_HPP_
#define SRC_TABLE_HPP_

#ifndef TABLE_HPP_
#define TABLE_HPP_

#include <libpq-fe.h>
#include <boost/format.hpp>
#include "columns.hpp"
#include <sstream>
#include <osmium/osm/types.hpp>
#include <geos/geom/Point.h>

/**
 * This class manages connection to a database table. We have one connection per table,
 * therefore this class is called Table, not DBConnection.
 */

class Table {
private:
    /**
     * name of the table
     */
    std::string m_name = "";

    /**
     * track if COPY mode has been entered
     */
    bool m_copy_mode = false;

    /**
     * track if a BEGIN COMMIT block has been opened
     */
    bool m_begin = false;

    Columns& m_columns;

    Config& m_config;

    /**
     * connection to database
     *
     * This pointer is a nullpointer if this table is used in demo mode (for testing purposes).
     */
    PGconn *m_database_connection;

    /**
     * maximum size of copy buffer
     */
    static const int BUFFER_SEND_SIZE = 10000;

    /**
     * create all necessary prepared statements for this table
     *
     * This method chooses the suitable prepared statements which are dependend from the table type (point vs. way vs. …).
     */
    void create_prepared_statements();

    /**
     * create a prepared statement
     */
    void create_prepared_statement(const char* name, std::string query, int params_count);

    /**
     * get ID of geometry column, first column is 0
     */
    int get_geometry_column_id();

public:
    Table() = delete;

    /**
     * constructor for production, establishes database connection
     */
    Table(const char* table_name, Config& config, Columns& columns);

    /**
     * constructor for testing, does not establishes database connection
     */
    Table(Columns& columns, Config& config);

    ~Table();


    Columns& get_columns();


    /**
     * send a line to the database (it will get it from STDIN) during copy mode
     */
    void send_line(const std::string& line);


    std::string& get_name() {
        return m_name;
    }

    /**
     * send BEGIN to table
     *
     * This method can be called both from inside the class and from outside (if you want to get your changes persistend after
     * you sent them all to the database.
     */
    void send_begin();

    /*
     * send COMMIT to table
     *
     * This method is intended to be called from the destructor of this class.
     */
    void commit();

    /**
     * Send any SQL query.
     *
     * This query will not return anything, i.e. it is useful for INSERT and DELETE operations.
     */
    void send_query(const char* query);
};


#endif /* TABLE_HPP_ */

