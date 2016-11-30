/*
 * vectortile_generator_config.hpp
 *
 *  Created on: 11.11.2016
 *      Author: michael
 */

#ifndef SRC_VECTORTILE_GENERATOR_CONFIG_HPP_
#define SRC_VECTORTILE_GENERATOR_CONFIG_HPP_


struct VectortileGeneratorConfig {
    std::string m_output_file = "/tmp/test.osm";
    std::string m_database = "pgimportertest";
    bool m_recurse_relations = false;
    bool m_recurse_ways = false;
    bool m_recurse_nodes = false;
    bool m_force = false;
    int m_x;
    int m_y;
    int m_zoom;
};




#endif /* SRC_VECTORTILE_GENERATOR_CONFIG_HPP_ */
