/*
 * osm_data_table.cpp
 *
 *  Created on:  2016-12-05
 *      Author: Michael Reichert
 */

#include <assert.h>
#include "osm_data_table.hpp"

OSMDataTable::OSMDataTable(const char* table_name, postgres_drivers::Config& config, postgres_drivers::Columns& columns) :
        postgres_drivers::Table(table_name, config, columns) {
    m_min_lon = new char[25];
    m_min_lat = new char[25];
    m_max_lon = new char[25];
    m_max_lat = new char[25];
}

OSMDataTable::~OSMDataTable() {
    // delete
    for (size_t i = 0; i < 4; ++i) {
        delete[] m_bbox_parameters[i];
    }
}

void OSMDataTable::set_bbox(const BoundingBox& bbox) {
    sprintf(m_min_lon, "%f", bbox.m_min_lon);
    sprintf(m_min_lat, "%f", bbox.m_min_lat);
    sprintf(m_max_lon, "%f", bbox.m_max_lon);
    sprintf(m_max_lat, "%f", bbox.m_max_lat);
    m_bbox_parameters[0] = m_min_lon;
    m_bbox_parameters[1] = m_min_lat;
    m_bbox_parameters[2] = m_max_lon;
    m_bbox_parameters[3] = m_max_lat;
#ifndef NDEBUG
    m_valid_bbox = true;
#endif
}

char** OSMDataTable::get_bbox_parameters() {
    return m_bbox_parameters;
}

void OSMDataTable::check_prepared_statement_execution(PGresult* result) {
    if ((PQresultStatus(result) != PGRES_COMMAND_OK) && (PQresultStatus(result) != PGRES_TUPLES_OK)) {
        std::string message = "Failed: ";
        message += PQresultErrorMessage(result);
        message += "\n";
        PQclear(result);
        throw std::runtime_error(message);
    }

}

PGresult* OSMDataTable::run_prepared_statement(const char* name, int param_count, const char* const * param_values) {
    assert(m_database_connection);
    PGresult* result = PQexecPrepared(m_database_connection, name, param_count, param_values, nullptr, nullptr, 0);
    check_prepared_statement_execution(result);
    return result;
}

PGresult* OSMDataTable::run_prepared_bbox_statement(const char* name) {
    assert(m_database_connection);
#ifndef NDEBUG
    assert(m_valid_bbox && "You must set the bounding box parameters before you can run queries!");
#endif
    PGresult* result = PQexecPrepared(m_database_connection, name, 4, m_bbox_parameters, nullptr, nullptr, 0);
    check_prepared_statement_execution(result);
    return result;
}
