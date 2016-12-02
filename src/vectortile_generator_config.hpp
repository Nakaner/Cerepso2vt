/*
 * vectortile_generator_config.hpp
 *
 *  Created on: 11.11.2016
 *      Author: michael
 */

#ifndef SRC_VECTORTILE_GENERATOR_CONFIG_HPP_
#define SRC_VECTORTILE_GENERATOR_CONFIG_HPP_


struct VectortileGeneratorConfig {
    std::string m_output_path = "/tmp/test.osm";
    std::string m_file_suffix = "osm.pbf";
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
    bool m_verbose = false;
    bool m_recurse_relations = false;
    bool m_recurse_ways = false;
    bool m_recurse_nodes = false;
    bool m_force = false;
    int m_x;
    int m_y;
    int m_zoom;
};




#endif /* SRC_VECTORTILE_GENERATOR_CONFIG_HPP_ */
