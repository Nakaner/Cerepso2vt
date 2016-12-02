/*
 * jobs_database.cpp
 *
 *  Created on:  2016-12-02
 *      Author: Michael Reichert
 */
#include <stdexcept>
#include <boost/format.hpp>
#include "jobs_database.hpp"

JobsDatabase::JobsDatabase(std::string& database_name) :
        m_name(database_name) {
    std::string connection_params = "dbname=";
    connection_params.append(database_name);
    m_database_connection = PQconnectdb(connection_params.c_str());
    if (PQstatus(m_database_connection) != CONNECTION_OK) {
        throw std::runtime_error((boost::format("Cannot establish connection to database: %1%\n")
            %  PQerrorMessage(m_database_connection)).str());
    }
    create_prepared_statements();
}

JobsDatabase::~JobsDatabase() {
    PQfinish(m_database_connection);
}

void JobsDatabase::create_prepared_statements() {
    std::string query = "INSERT INTO jobs (quadtree_id, created, path) VALUES ($1, $2, $3)";
    create_prepared_statement("insert_job", query, 3);
    query = "DELETE FROM jobs WHERE quadtree_id = $1 AND processing = false";
    create_prepared_statement("drop_job", query, 1);
}

void JobsDatabase::create_prepared_statement(const char* name, std::string& query, int params_count) {
    PGresult *result = PQprepare(m_database_connection, name, query.c_str(), params_count, NULL);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        PQclear(result);
        throw std::runtime_error((boost::format("%1% failed: %2%\n") % query % PQerrorMessage(m_database_connection)).str());
    }
    PQclear(result);
}

void JobsDatabase::add_job(int x, int y, int zoom, const char* created, const char* vectortile_path) {
    // convert tile ID to quadtree ID
    int64_t quadtree_tile_id = xy_to_quadtree(x, y, zoom);
    char const *param_values[3];
    char buffer_qt[20];
    sprintf(buffer_qt, "%ld", quadtree_tile_id);
    param_values[0] = buffer_qt;
    param_values[1] = created;
    param_values[2] = vectortile_path;

    PGresult *result = PQexecPrepared(m_database_connection, "insert_job", 3, param_values, nullptr, nullptr, 0);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        throw std::runtime_error((boost::format("Inserting job %1%/%2%/%3% into the jobs database failed: %4%\n") % zoom
                % x % y % PQresultErrorMessage(result)).str());
        PQclear(result);
    }
    PQclear(result);
}

void JobsDatabase::cancel_job(int x, int y, int zoom) {
    int64_t quadtree_tile_id = xy_to_quadtree(x, y, zoom);
    char const *param_values[1];
    char buffer_qt[20];
    sprintf(buffer_qt, "%ld", quadtree_tile_id);
    param_values[0] = buffer_qt;

    PGresult *result = PQexecPrepared(m_database_connection, "drop_job", 1, param_values, nullptr, nullptr, 0);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        throw std::runtime_error((boost::format("Removing job %1%/%2%/%3% from the jobs database failed: %4%\n") % zoom
                % x % y % PQresultErrorMessage(result)).str());
        PQclear(result);
    }
    PQclear(result);
}

/*static*/ int64_t JobsDatabase::xy_to_quadtree(int x, int y, int zoom) {
    int64_t qt = 0;
    // the two highest bits are the bits of zoom level 1, the third and fourth bit are level 2, …
    for (int z = 0; z < zoom; z++) {
        qt = qt + ((x & (1 << z)) << z);
        qt = qt + ((y & (1 << z)) << (z+1));
    }
    return qt;
}
