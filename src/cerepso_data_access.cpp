/*
 * cerepso_data_access.cpp
 *
 *  Created on:  2019-08-19
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "cerepso_data_access.hpp"

CerepsoDataAccess::CerepsoDataAccess(VectortileGeneratorConfig& config, OSMDataTable& untagged_nodes_table,
        OSMDataTable& nodes_table, OSMDataTable& ways_table, OSMDataTable& relations_table) :
    m_config(config),
    m_untagged_nodes_table(untagged_nodes_table),
    m_nodes_table(nodes_table),
    m_ways_table(ways_table),
    m_relations_table(relations_table) {
    create_prepared_statements(config);
}

void CerepsoDataAccess::create_prepared_statements(const VectortileGeneratorConfig& config) {
    std::string query;
    query = (boost::format("SELECT tags, osm_id, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE ST_INTERSECTS(geom, " \
            "ST_MakeEnvelope($1, $2, $3, $4, 4326))") % m_nodes_table.get_name()).str();
    m_nodes_table.create_prepared_statement("get_nodes_with_tags", query, 4);
    query = (boost::format("SELECT tags, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE osm_id = $1") % m_nodes_table.get_name()).str();
    m_nodes_table.create_prepared_statement("get_single_node_with_tags", query, 1);

    // retrieval of untagged nodes by location is not possible without a geometry column
    if (config.m_untagged_nodes_geom) {
        query = (boost::format("SELECT osm_id, osm_uid, osm_version, osm_lastmodified," \
                " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE ST_INTERSECTS(geom, " \
                "ST_MakeEnvelope($1, $2, $3, $4, 4326))") % m_untagged_nodes_table.get_name()).str();
        m_untagged_nodes_table.create_prepared_statement("get_nodes_without_tags", query, 4);
    }
    if (config.m_untagged_nodes_geom) {
        query = (boost::format("SELECT osm_uid, osm_version, osm_lastmodified," \
                " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE osm_id = $1") % m_untagged_nodes_table.get_name()).str();
    } else {
        query = (boost::format("SELECT osm_uid, osm_version, osm_lastmodified," \
                " osm_changeset, x, y FROM %1% WHERE osm_id = $1") % m_untagged_nodes_table.get_name()).str();
    }
    m_untagged_nodes_table.create_prepared_statement("get_single_node_without_tags", query, 1);

    query = (boost::format("SELECT tags, osm_id, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, way_nodes FROM %1% WHERE ST_INTERSECTS(geom, " \
            "ST_MakeEnvelope($1, $2, $3, $4, 4326))") % m_ways_table.get_name()).str();
    m_ways_table.create_prepared_statement("get_ways", query, 4);
    query = (boost::format("SELECT tags, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, way_nodes FROM %1% WHERE osm_id = $1") % m_ways_table.get_name()).str();
    m_ways_table.create_prepared_statement("get_single_way", query, 1);

    query = (boost::format("SELECT tags, osm_id, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, member_ids, member_types, member_roles FROM %1% WHERE ST_INTERSECTS(geom_points, " \
            "ST_MakeEnvelope($1, $2, $3, $4, 4326)) OR ST_INTERSECTS(geom_lines, ST_MakeEnvelope($1, $2, $3, $4, 4326))")
            % m_relations_table.get_name()).str();
    m_relations_table.create_prepared_statement("get_relations", query, 4);
    query = (boost::format("SELECT tags, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, member_ids, member_types, member_roles FROM %1% WHERE osm_id = $1")
            % m_relations_table.get_name()).str();
    m_relations_table.create_prepared_statement("get_single_relation", query, 4);
}

void CerepsoDataAccess::set_bbox(const BoundingBox& bbox) {
    m_nodes_table.set_bbox(bbox);
    m_untagged_nodes_table.set_bbox(bbox);
    m_ways_table.set_bbox(bbox);
    m_relations_table.set_bbox(bbox);
}

void CerepsoDataAccess::set_add_node_callback(osm_vector_tile_impl::node_callback_type&& callback) {
    m_add_node_callback = callback;
}

void CerepsoDataAccess::set_add_way_callback(osm_vector_tile_impl::way_callback_type&& callback) {
    m_add_way_callback = callback;
}

void CerepsoDataAccess::set_add_relation_callback(osm_vector_tile_impl::relation_callback_type&& callback) {
    m_add_relation_callback = callback;
}

void CerepsoDataAccess::parse_relation_query_result(PGresult* result, const osmium::object_id_type id) {
    int tuple_count = PQntuples(result);
    int field_offset = 0; // will be added to all field IDs if we query a tags column
    if (id == 0) {
        field_offset = 1;
    }
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        std::string tags_hstore = PQgetvalue(result, i, 0);
        std::string member_ids = PQgetvalue(result, i, 5 + field_offset);
        std::string member_types = PQgetvalue(result, i, 6 + field_offset);
        std::string member_roles = PQgetvalue(result, i, 7 + field_offset);
        const osmium::object_id_type osm_id = (id == 0) ? id : strtoll(PQgetvalue(result, i, 1), nullptr, 9 + field_offset);
        const char* changeset = PQgetvalue(result, i, 4 + field_offset);
        const char* uid = PQgetvalue(result, i, 1 + field_offset);
        const char* version = PQgetvalue(result, i, 2 + field_offset);
        const char* timestamp = PQgetvalue(result, i, 3 + field_offset);
        m_add_relation_callback(osm_id, std::move(member_ids), std::move(member_types),
                std::move(member_roles), version, changeset, uid, timestamp,
                std::move(tags_hstore));
    }
}

void CerepsoDataAccess::get_missing_relations(const osm_vector_tile_impl::osm_id_set_type& missing_relations) {
    char* param_values[1];
    char param[25];
    param_values[0] = param;
    for (const auto id : missing_relations) {
        sprintf(param, "%ld", id);
        PGresult* result = m_ways_table.run_prepared_statement("get_single_relation", 1, param_values);
        parse_relation_query_result(result, id);
        PQclear(result);
    }
//    m_buffer.commit();
}

void CerepsoDataAccess::get_nodes_inside() {
    PGresult* result = m_nodes_table.run_prepared_bbox_statement("get_nodes_with_tags");
    parse_node_query_result(result, true, 0);
    PQclear(result);
    if (m_config.m_orphaned_nodes) { // If requested by the user, query untagged nodes table, too.
        PGresult* result = m_untagged_nodes_table.run_prepared_bbox_statement("get_nodes_without_tags");
        parse_node_query_result(result, false, 0);
        PQclear(result);
    }
}

void CerepsoDataAccess::get_missing_nodes(const osm_vector_tile_impl::osm_id_set_type& missing_nodes) {
    char* param_values[1];
    char param[25];
    param_values[0] = param;
    for (const osmium::object_id_type id : missing_nodes) {
        sprintf(param, "%ld", id);
        PGresult* result = m_untagged_nodes_table.run_prepared_statement("get_single_node_without_tags", 1, param_values);
        if (PQntuples(result) > 0) {
            parse_node_query_result(result, false, id);
        } else {
            PGresult* result2 = m_nodes_table.run_prepared_statement("get_single_node_with_tags", 1, param_values);
            parse_node_query_result(result2, true, id);
            PQclear(result2);
        }
        PQclear(result);
    }
}

void CerepsoDataAccess::parse_node_query_result(PGresult* result, const bool with_tags,
        const osmium::object_id_type id) {
    int field_offset = 0; // will be added to all field IDs if we query a tags column
    if (with_tags && id == 0) {
        field_offset = 2;
    } else if ((with_tags && id != 0) || (!with_tags && id == 0)) {
        field_offset = 1;
    }
    int tuple_count = PQntuples(result);
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        double x, y;
        if (!with_tags && !m_config.m_untagged_nodes_geom) {
            x = osmium::Location::fix_to_double(atoi(PQgetvalue(result, i, 4 + field_offset)));
            y = osmium::Location::fix_to_double(atoi(PQgetvalue(result, i, 5 + field_offset)));
        } else {
            x = atof(PQgetvalue(result, i, 4 + field_offset));
            y = atof(PQgetvalue(result, i, 5 + field_offset));
        }
        osmium::Location location{x, y};
        osmium::object_id_type osm_id;

        if (id == 0 && with_tags) {
            osm_id = strtoll(PQgetvalue(result, i, 1), nullptr, 10);
        } else if (id == 0 && !with_tags) {
            osm_id = strtoll(PQgetvalue(result, i, 0), nullptr, 10);
        } else {
            osm_id = id;
        }
        const char* version = PQgetvalue(result, i, 1 + field_offset);
        const char* uid = PQgetvalue(result, i, 0 + field_offset);
        const char* timestamp = PQgetvalue(result, i, 2 + field_offset);
        const char* changeset = PQgetvalue(result, i, 3 + field_offset);
        if (with_tags) {
            std::string tags_hstore = PQgetvalue(result, i, 0);
            m_add_node_callback(osm_id, location, version, changeset, uid, timestamp, std::move(tags_hstore));
        } else {
            m_add_node_callback(osm_id, location, version, changeset, uid, timestamp, "");
        }
    }
}

void CerepsoDataAccess::parse_way_query_result(PGresult* result, const osmium::object_id_type id) {
    int tuple_count = PQntuples(result);
    int field_offset = 0; // will be added to all field IDs if we query a tags column
    if (id == 0) {
        field_offset = 1;
    }
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        std::string tags_hstore = PQgetvalue(result, i, 0);
        std::string way_nodes = PQgetvalue(result, i, 5 + field_offset);
        osmium::object_id_type way_id = id;
        if (id == 0) {
            way_id = strtoll(PQgetvalue(result, i, 1), nullptr, 10);
        }
        const char* changeset = PQgetvalue(result, i, 4 + field_offset);
        const char* uid = PQgetvalue(result, i, 1 + field_offset);
        const char* version = PQgetvalue(result, i, 2 + field_offset);
        const char* timestamp = PQgetvalue(result, i, 3+ field_offset);
        m_add_way_callback(way_id, std::move(way_nodes), version, changeset, uid, timestamp, std::move(tags_hstore));
    }
}

void CerepsoDataAccess::get_ways_inside() {
    PGresult* result = m_ways_table.run_prepared_bbox_statement("get_ways");
    parse_way_query_result(result, 0);
    PQclear(result);
}

void CerepsoDataAccess::get_missing_ways(const osm_vector_tile_impl::osm_id_set_type& missing_ways) {
    char* param_values[1];
    char param[25];
    param_values[0] = param;
    for (const osmium::object_id_type id : missing_ways) {
        sprintf(param, "%ld", id);
        PGresult* result = m_ways_table.run_prepared_statement("get_single_way", 1, param_values);
        parse_way_query_result(result, id);
        PQclear(result);
    }
}

void CerepsoDataAccess::get_relations_inside() {
    PGresult* result = m_relations_table.run_prepared_bbox_statement("get_relations");
    parse_relation_query_result(result, 0);
    PQclear(result);
}


