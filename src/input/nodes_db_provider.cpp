/*
 * untagged_nodes_db_provider.cpp
 *
 *  Created on:  2019-10-09
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "nodes_db_provider.hpp"
#include <libpq-fe.h>
#include <string>

input::NodesDBProvider::NodesDBProvider(VectortileGeneratorConfig& config,
        OSMDataTable&& nodes_table, OSMDataTable&& untagged_nodes_table) :
    NodesProvider(config, std::move(nodes_table)),
    m_config(config),
    m_untagged_nodes_table(std::move(untagged_nodes_table)) {
    create_prepared_statements_untagged();
}

input::NodesDBProvider::~NodesDBProvider() {
}

void input::NodesDBProvider::create_prepared_statements_untagged() {
    std::string query = m_metadata.select_str();
    // retrieval of untagged nodes by location is not possible without a geometry column
    if (m_config.m_untagged_nodes_geom) {
        std::string geom_column_name = m_nodes_table.get_column_name_by_type(postgres_drivers::ColumnType::POINT);
        query = m_metadata.select_str();
        query += " osm_id, ST_X(%1%), ST_Y(%1%)";
        query.append(" FROM %2% WHERE ST_INTERSECTS(%1%, ST_MakeEnvelope($1, $2, $3, $4, 4326))");
        query = (boost::format(query) % geom_column_name % m_untagged_nodes_table.get_name()).str();
        m_nodes_table.create_prepared_statement("get_nodes_without_tags", query, 1);

        query = m_metadata.select_str();
        query += " ST_X(%1%), ST_Y(%1%)";
        query.append(" FROM %2% WHERE osm_id = $1");
        query = (boost::format(query) % geom_column_name % m_untagged_nodes_table.get_name()).str();
    } else {
        query = m_metadata.select_str();
        query += " x, y";
        query.append(" FROM %1% WHERE osm_id = $1");
        query = (boost::format(query) % m_untagged_nodes_table.get_name()).str();
    }
    m_untagged_nodes_table.create_prepared_statement("get_single_node_without_tags", query, 1);
}

void input::NodesDBProvider::set_bbox(const BoundingBox& bbox) {
    NodesProvider::set_bbox(bbox);
    m_untagged_nodes_table.set_bbox(bbox);
}

void input::NodesDBProvider::get_nodes_inside() {
    NodesProvider::get_nodes_inside();
    if (m_config.m_orphaned_nodes) { // If requested by the user, query untagged nodes table, too.
        PGresult* result = m_untagged_nodes_table.run_prepared_bbox_statement("get_nodes_without_tags");
        parse_node_query_result(result, false, 0);
        PQclear(result);
    }
}

void input::NodesDBProvider::get_missing_nodes(const osm_vector_tile_impl::osm_id_set_type& missing_nodes) {
    char* param_values[1];
    char param[25];
    param_values[0] = param;
    for (const osmium::object_id_type id : missing_nodes) {
        sprintf(param, "%ld", id);
        PGresult* result = m_untagged_nodes_table.run_prepared_statement("get_single_node_without_tags", 1, param_values);
        if (PQntuples(result) > 0) {
            parse_node_query_result(result, false, id);
            PQclear(result);
            continue;
        }
        PQclear(result);
        get_node_with_tags(id, param_values);
    }
}
