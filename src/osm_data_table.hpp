/*
 * osm_data_table.hpp
 *
 *  Created on:  2016-12-05
 *      Author: Michael Reichert
 */

#ifndef SRC_OSM_DATA_TABLE_HPP_
#define SRC_OSM_DATA_TABLE_HPP_

#include <libpq-fe.h>
#include <postgres_drivers/table.hpp>
#include "bounding_box.hpp"

/**
 * \brief This class extends the features offered by the postgres-drivers library.
 *
 * Because we do a lot of bounding-box based querys (`ST_Intersects`) and use the instances of this class
 * for multiple tiles we create, it is a good idea to store query parameters which are used very often
 * permanent in this class.
 *
 * This class aims to be as independent as possible from the output format of the vector tiles and is designed
 * as a wrapper around postgres_drivers::Table.
 */

class OSMDataTable : public postgres_drivers::Table {
private:
    /**
     * \brief array which contains pointers to the four parameters of the bounding box used
     * for prepared statements which have only these four parameters
     */
    char* m_bbox_parameters[4];
    std::unique_ptr<char> m_min_lon;
    std::unique_ptr<char> m_min_lat;
    std::unique_ptr<char> m_max_lon;
    std::unique_ptr<char> m_max_lat;

#ifndef NDEBUG
    /**
     * Check if bounding box parameters are valid. This check is not done in release builds
     * (therefore surrounded by \c \#ifndef NDEBUG).
     */
    bool m_valid_bbox = false;
#endif

    /**
     * \brief Check if the execution of a prepared statement worked as expected and throw an exception if not.
     *
     * \param result result of that prepared statement
     */
    void check_prepared_statement_execution(PGresult* result);

public:
    OSMDataTable(const char* table_name, postgres_drivers::Config& config, postgres_drivers::Columns& columns);

    /**
     * set/change the bounding box which is currently used
     *
     * \param bbox reference to an instance of BoudingBox
     */
    void set_bbox(const BoundingBox& bbox);

    /**
     * \brief Get a pointer to the array of bounding box parameters.
     *
     * Ownership will not be transferred. The destructor of this class will clean up the memory.
     */
    char** get_bbox_parameters();

    /**
     * \brief execute a prepared statement
     *
     * The statement has to be registered first using postgres_drivers::Table::create_prepared_statement(const char*, std::string, int).
     *
     * \param name name of the prepared statement
     * \param param_count number of parameters of this prepared statement
     * \param param_values parameters
     *
     * \returns result of the query. You get ownership of the memory and have to call PQclear(PGresult*) to destroy it.
     */
    PGresult* run_prepared_statement(const char* name, int param_count, const char* const * param_values);

    /**
     * \brief execute a prepared statement using a spatial query
     *
     * The statement has to be registered first using postgres_drivers::Table::create_prepared_statement(const char*, std::string, int).
     * It must have only four parameters and its where clause should only contain one condition: ST_Intersects()
     *
     * \param name name of the prepared statement
     *
     * \returns result of the query. You get ownership of the memory and have to call PQclear(PGresult*) to destroy it.
     */
    PGresult* run_prepared_bbox_statement(const char* name);

};



#endif /* SRC_OSM_DATA_TABLE_HPP_ */
