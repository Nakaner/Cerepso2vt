/*
 * untagged_nodes_flatnode_provider.cpp
 *
 *  Created on:  2019-10-10
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "nodes_flatnode_provider.hpp"

input::cerepso::dense_file_array_t input::cerepso::NodesFlatnodeProvider::load_index(const char* path) {
    if (!path || *path == '\0') {
        throw std::runtime_error{"No flatnodes path provided."};
    }
    int fd = ::open(path, O_RDWR);
    if (fd == -1) {
        std::string err = "failed to open flatnodes file ";
        err += path;
        err += ": ";
        err += std::strerror(errno);
        throw std::runtime_error{err};
    }
    return dense_file_array_t{fd};
}

input::cerepso::NodesFlatnodeProvider::NodesFlatnodeProvider(
        VectortileGeneratorConfig& config, const char* nodes_table_name) :
    NodesProvider(config, nodes_table_name),
    m_storage_pos(load_index(config.m_flatnodes_path.c_str())),
    m_location_handler(m_storage_pos) {
}

input::cerepso::NodesFlatnodeProvider::~NodesFlatnodeProvider() {
}

void input::cerepso::NodesFlatnodeProvider::get_nodes_inside() {
    NodesProvider::get_nodes_inside();
    if (m_config.m_orphaned_nodes) { // If requested by the user, query untagged nodes table, too.
        throw std::runtime_error{"Cannot query untagged nodes in the flatnodes file by bounding box."};
    }
}

void input::cerepso::NodesFlatnodeProvider::get_missing_nodes(const osm_vector_tile_impl::osm_id_set_type& missing_nodes) {
    char* param_values[1];
    char param[25];
    param_values[0] = param;
    for (const osmium::object_id_type id : missing_nodes) {
        sprintf(param, "%ld", id);
        if (!get_node_with_tags(id, param_values)) {
            osmium::Location location = m_location_handler.get_node_location(id);
            if (location.valid()) {
                m_add_simple_node_callback(id, location);
            }
        }
    }
}
