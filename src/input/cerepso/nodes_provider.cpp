/*
 * untagged_nodes_provider.cpp
 *
 *  Created on:  2019-10-09
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "nodes_provider.hpp"
#include <string>


input::cerepso::NodesProvider::NodesProvider(VectortileGeneratorConfig& config, OSMDataTable& nodes_table) :
    m_config(config),
    m_nodes_table(nodes_table),
    m_metadata(config) {
    create_prepared_statements();
}

input::cerepso::NodesProvider::~NodesProvider() {
}

void input::cerepso::NodesProvider::set_add_node_callback(osm_vector_tile_impl::node_callback_type& callback) {
    m_add_node_callback = callback;
}

void input::cerepso::NodesProvider::set_add_simple_node_callback(osm_vector_tile_impl::simple_node_callback_type& callback) {
    m_add_simple_node_callback = callback;
}

void input::cerepso::NodesProvider::set_bbox(const BoundingBox& bbox) {
    m_nodes_table.set_bbox(bbox);
}

void input::cerepso::NodesProvider::create_prepared_statements() {
    std::string query = m_metadata.select_str();
    query += " osm_id, tags, ST_X(geom), ST_Y(geom) FROM %1% WHERE ST_INTERSECTS(geom, ST_MakeEnvelope($1, $2, $3, $4, 4326))";
    query = (boost::format(query) % m_nodes_table.get_name()).str();
    m_nodes_table.create_prepared_statement("get_nodes_with_tags", query, 4);

    query = m_metadata.select_str();
    query += " tags, ST_X(geom), ST_Y(geom)";
    query.append(" FROM %1% WHERE osm_id = $1");
    query = (boost::format(query) % m_nodes_table.get_name()).str();
    m_nodes_table.create_prepared_statement("get_single_node_with_tags", query, 1);
}

void input::cerepso::NodesProvider::get_nodes_inside() {
    PGresult* result = m_nodes_table.run_prepared_bbox_statement("get_nodes_with_tags");
    parse_node_query_result(result, true, 0);
    PQclear(result);
}

bool input::cerepso::NodesProvider::get_node_with_tags(const osmium::object_id_type id, char** param_values) {
    PGresult* result = m_nodes_table.run_prepared_statement("get_single_node_with_tags", 1, param_values);
    if (PQntuples(result) > 0) {
        parse_node_query_result(result, true, id);
        PQclear(result);
        return true;
    }
    PQclear(result);
    return false;
}

/*static*/ void input::cerepso::NodesProvider::parse_node_query_result(PGresult* result, const bool with_tags,
        const osmium::object_id_type id) {
    int id_field_offset = m_metadata.count();
    int tags_field_offset = m_metadata.count();
    if (id == 0) {
        ++tags_field_offset;
    }
    int other_field_offset = with_tags ? tags_field_offset + 1 : tags_field_offset;
    int tuple_count = PQntuples(result);
    for (int i = 0; i < tuple_count; ++i) { // for each returned row
        double x, y;
        if (!with_tags && !m_config.m_untagged_nodes_geom) {
            x = osmium::Location::fix_to_double(atoi(PQgetvalue(result, i, other_field_offset)));
            y = osmium::Location::fix_to_double(atoi(PQgetvalue(result, i, other_field_offset + 1)));
        } else {
            x = atof(PQgetvalue(result, i, other_field_offset));
            y = atof(PQgetvalue(result, i, other_field_offset + 1));
        }
        osmium::Location location{x, y};
        osmium::object_id_type osm_id = id;
        if (id == 0) {
            osm_id = strtoll(PQgetvalue(result, i, id_field_offset), nullptr, 10);
        }
        const char* version = m_metadata.has_version() ? PQgetvalue(result, i, m_metadata.version()) : nullptr;
        const char* uid = m_metadata.has_uid() ? PQgetvalue(result, i, m_metadata.uid()) : nullptr;
        const char* timestamp = m_metadata.has_last_modified() ? PQgetvalue(result, i, m_metadata.last_modified()) : nullptr;
        const char* changeset = m_metadata.has_changeset() ? PQgetvalue(result, i, m_metadata.changeset()) : nullptr;
        if (with_tags) {
            std::string tags_hstore = PQgetvalue(result, i, tags_field_offset);
            m_add_node_callback(osm_id, location, version, changeset, uid, timestamp, std::move(tags_hstore));
        } else {
            m_add_node_callback(osm_id, location, version, changeset, uid, timestamp, "");
        }
    }
}
