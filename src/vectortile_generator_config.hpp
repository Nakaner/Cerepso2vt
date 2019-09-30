/*
 * vectortile_generator_config.hpp
 *
 *  Created on: 11.11.2016
 *      Author: michael
 */

#ifndef SRC_VECTORTILE_GENERATOR_CONFIG_HPP_
#define SRC_VECTORTILE_GENERATOR_CONFIG_HPP_


struct VectortileGeneratorConfig {
    /// output path
    std::string m_output_path = "-";
    /// default file suffix, determines file format in single-tile mode
    std::string m_file_suffix = "osm.pbf";
    /// default name of PostgreSQL database to be queried
    std::string m_database = "pgimportertest";

    /**
     * \brief name of the database where the processing jobs are managed
     *
     * If it is an empty string, no jobs are written to any database.
     */
    std::string m_jobs_database = "";

    /// true if we create multiple tiles at once
    bool m_batch_mode = false;

    /// Do a spatial query on `untagged_nodes` table?
    bool m_orphaned_nodes = false;
    /// be verbose on command line
    bool m_verbose = false;

    /**
     * \brief Fetch relations from the database which are
     * not included in the tile but referenced by relations
     * which are included in the tile?
     *
     * See usage documentation for details.
     */
    bool m_recurse_relations = false;

    /**
     * \brief Fetch ways from the database which are not included in
     * the tile but referenced by relations which are included in the
     * tile?
     *
     * See usage documentation for details.
     */
    bool m_recurse_ways = false;

    /**
     * \brief Fetch nodes outside the tile which are missing?
     *
     * See usage documentation for details.
     */
    bool m_recurse_nodes = false;

    /**
     * \brief overwrite output file(s)?
     */
    bool m_force = false;

    /// x index of the tile to be generated
    int m_x;
    /// y index of the tile to be generated
    int m_y;
    /// zoom level of the tile to be generated
    int m_zoom;

    /**
     *  Cerepso database layout related: Does the untagged_nodes table contain a geometry field
     *  or two distinct x and y columns of type int32?
     */
    bool m_untagged_nodes_geom = false;
};




#endif /* SRC_VECTORTILE_GENERATOR_CONFIG_HPP_ */
