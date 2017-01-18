/*
 * jobs_database.hpp
 *
 *  Created on:  2016-12-02
 *      Author: Michael Reichert
 */

#ifndef SRC_JOBS_DATABASE_HPP_
#define SRC_JOBS_DATABASE_HPP_

#include <libpq-fe.h>
#include <string>

/**
 * \brief struct for functions returning a x and a y coordinates by one call
 */
struct xy_coord_t {
    int x = 0;
    int y = 0;
    xy_coord_t(int x_coord, int y_coord) : x(x_coord), y(y_coord) {}
    xy_coord_t() : xy_coord_t(0, 0) {}
};

/**
 * \brief manage connection to the database which manages the jobs
 *
 * The job database has one table with three columns:
 * * quadtree_id bigint -- ID of a tile as quadtree ID
 * * processing bool DEFAULT FALSE -- if the processing has started (this fields serves as a locking mechanism)
 * * created char(21) -- date of creation (for debugging purposes and if you want to create statistics)
 * * path text -- where the tile is saved on the disk
 *
 * Before calling the constructor of this class, the database and the table jobs has to be created:
 * ```CREATE TABLE jobs (quadtree_id bigint, processing bool DEFAULT FALSE, created char(21), path text);
 * CREATE INDEX quadtree_id_idx ON jobs USING btree(quadtree_id);```
 */
class JobsDatabase {
    /**
     * \brief connection to database
     */
    PGconn *m_database_connection;

    /**
     * database name
     */
    std::string& m_name;

    /**
     * \brief create some prepared statements for faster writing
     */
    void create_prepared_statements();

    /**
     * \brief create a prepared statement
     *
     * \param name name of the statement
     * \param query query template
     * \param params_count number of parameters
     */
    void create_prepared_statement(const char* name, std::string& query, int params_count);

public:
    /**
     * constructor
     *
     * \param database_name name of the database
     */
    JobsDatabase(std::string& database_name);

    /**
     * close connection to database, clean up memory
     */
    ~JobsDatabase();

    /**
     * \brief add a job to the database
     *
     * \param x x index of the tile
     * \param y y index of the tile
     * \param zoom zoom level of the tile
     * \param created creation date as ISO timestring (YYYY-MM-DDThh:mm:ssZ). This should be a null-terminated C string.
     * \param vectortile_path Location of the vector tile as null-terminated C string. This is a relative path relative to the base directory of the produced tiles.
     */
    void add_job(int x, int y, int zoom, const char* created, const char* vectortile_path);

    /**
     * \brief cancel a job which has not been processed yet
     *
     * Jobs whose processing has started yet will not be cancelled.
     *
     * \param x x index of the tile
     * \param y y index of the tile
     * \param zoom zomm level of the tile
     */
    void cancel_job(int x, int y, int zoom);

    /**
     * \brief Convert a tile ID (x and y) into quadtree coordinate using bitshifts.
     *
     * Quadtree coordinates are interleaved this way: YXYX…
     *
     * \param x x index
     * \param y y index
     * \param zoom zoom level
     * \returns quadtree ID as int64_t
     */
    static int64_t xy_to_quadtree(int x, int y, int zoom);
};


#endif /* SRC_JOBS_DATABASE_HPP_ */
