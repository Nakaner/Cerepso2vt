/*
 * cerepso_data_access.cpp
 *
 *  Created on:  2019-08-19
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "cerepso_data_access.hpp"
#include <algorithm>

CerepsoDataAccess::CerepsoDataAccess(VectortileGeneratorConfig& config, OSMDataTable& untagged_nodes_table,
        OSMDataTable& nodes_table, OSMDataTable& ways_table, OSMDataTable& relations_table,
        OSMDataTable& node_ways_table, OSMDataTable& node_relations_table,
        OSMDataTable& way_relations_table, OSMDataTable& relation_relations_table) :
    m_config(config),
    m_untagged_nodes_table(untagged_nodes_table),
    m_nodes_table(nodes_table),
    m_ways_table(ways_table),
    m_relations_table(relations_table),
    m_node_ways_table(node_ways_table),
    m_node_relations_table(node_relations_table),
    m_way_relations_table(way_relations_table),
    m_relation_relations_table(relation_relations_table) {
    set_metadata_field_count();
    create_prepared_statements();
}

void CerepsoDataAccess::set_metadata_field_count() {
    m_metadata_field_count = 0;
    if (m_config.m_postgres_config.metadata.user()) {
        m_user_index = m_metadata_field_count;
        ++m_metadata_field_count;
    }
    if (m_config.m_postgres_config.metadata.uid()) {
        m_uid_index = m_metadata_field_count;
        ++m_metadata_field_count;
    }
    if (m_config.m_postgres_config.metadata.version()) {
        m_version_index = m_metadata_field_count;
        ++m_metadata_field_count;
    }
    if (m_config.m_postgres_config.metadata.timestamp()) {
        m_last_modified_index = m_metadata_field_count;
        ++m_metadata_field_count;
    }
    if (m_config.m_postgres_config.metadata.changeset()) {
        m_changeset_index = m_metadata_field_count;
        ++m_metadata_field_count;
    }
}

std::string CerepsoDataAccess::metadata_select_str() {
    std::string query = "SELECT ";
    if (m_config.m_postgres_config.metadata.user()) {
        query.append("osm_user, ");
    }
    if (m_config.m_postgres_config.metadata.uid()) {
        query.append("osm_uid, ");
    }
    if (m_config.m_postgres_config.metadata.version()) {
        query.append("osm_version, ");
    }
    if (m_config.m_postgres_config.metadata.timestamp()) {
        query.append("osm_lastmodified, ");
    }
    if (m_config.m_postgres_config.metadata.changeset()) {
        query.append("osm_changeset, ");
    }
    return query;
}

void CerepsoDataAccess::create_prepared_statements() {
    // nodes
    std::string query = metadata_select_str();
    query += " osm_id, tags, ST_X(geom), ST_Y(geom) FROM %1% WHERE ST_INTERSECTS(geom, ST_MakeEnvelope($1, $2, $3, $4, 4326))";
    query = (boost::format(query) % m_nodes_table.get_name()).str();
    m_nodes_table.create_prepared_statement("get_nodes_with_tags", query, 4);

    query = metadata_select_str();
    query += " tags, ST_X(geom), ST_Y(geom)";
    query.append(" FROM %1% WHERE osm_id = $1");
    query = (boost::format(query) % m_nodes_table.get_name()).str();
    m_nodes_table.create_prepared_statement("get_single_node_with_tags", query, 1);

    // retrieval of untagged nodes by location is not possible without a geometry column
    if (m_config.m_untagged_nodes_geom) {
        query = metadata_select_str();
        query += " osm_id, ST_X(geom), ST_Y(geom)";
        query.append(" FROM %1% WHERE ST_INTERSECTS(geom, ST_MakeEnvelope($1, $2, $3, $4, 4326))");
        query = (boost::format(query) % m_untagged_nodes_table.get_name()).str();
        m_nodes_table.create_prepared_statement("get_nodes_without_tags", query, 1);
    }
    if (m_config.m_untagged_nodes_geom) {
        query = metadata_select_str();
        query += " ST_X(geom), ST_Y(geom)";
        query.append(" FROM %1% WHERE osm_id = $1");
        query = (boost::format(query) % m_untagged_nodes_table.get_name()).str();
    } else {
        query = metadata_select_str();
        query += " x, y";
        query.append(" FROM %1% WHERE osm_id = $1");
        query = (boost::format(query) % m_untagged_nodes_table.get_name()).str();
    }
    m_untagged_nodes_table.create_prepared_statement("get_single_node_without_tags", query, 1);

    // ways
    query = metadata_select_str();
    query += " osm_id, tags";
    query.append(" FROM %1% WHERE ST_INTERSECTS(geom, ST_MakeEnvelope($1, $2, $3, $4, 4326))");
    query = (boost::format(query) % m_ways_table.get_name()).str();
    m_ways_table.create_prepared_statement("get_ways", query, 4);

    query = metadata_select_str();
    query += " tags";
    query.append(" FROM %1% WHERE osm_id = $1");
    query = (boost::format(query) % m_ways_table.get_name()).str();
    m_ways_table.create_prepared_statement("get_single_way", query, 1);

    query = (boost::format("SELECT node_id, position FROM %1% WHERE way_id = $1")
        % m_node_ways_table.get_name()).str();
    m_node_ways_table.create_prepared_statement("get_way_nodes", query, 1);

    // relations
    query = metadata_select_str();
    query += " osm_id, tags";
    query.append(" FROM %1% WHERE ST_INTERSECTS(geom_points, " \
            "ST_MakeEnvelope($1, $2, $3, $4, 4326)) OR ST_INTERSECTS(geom_lines, ST_MakeEnvelope($1, $2, $3, $4, 4326))");
    query = (boost::format(query) % m_relations_table.get_name()).str();
    m_relations_table.create_prepared_statement("get_relations", query, 4);

    query = metadata_select_str();
    query += " tags";
    query.append(" FROM %1% WHERE osm_id = $1");
    query = (boost::format(query) % m_relations_table.get_name()).str();
    m_ways_table.create_prepared_statement("get_single_relation", query, 1);

    std::string get_rel_members_template = "SELECT member_id, role, position FROM %1% WHERE relation_id = $1";
    query = (boost::format(get_rel_members_template) % m_node_relations_table.get_name()).str();
    m_node_relations_table.create_prepared_statement("get_relation_members", query, 1);
    query = (boost::format(get_rel_members_template) % m_way_relations_table.get_name()).str();
    m_way_relations_table.create_prepared_statement("get_relation_members", query, 1);
    query = (boost::format(get_rel_members_template) % m_relation_relations_table.get_name()).str();
    m_relation_relations_table.create_prepared_statement("get_relation_members", query, 1);
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
    int id_field_offset = m_metadata_field_count;
    int tags_field_offset = id_field_offset;
    if (id == 0) {
        ++tags_field_offset;
    }
    int tuple_count = PQntuples(result);
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        std::string tags_hstore = PQgetvalue(result, i, tags_field_offset);
        const osmium::object_id_type osm_id = (id == 0) ? strtoll(PQgetvalue(result, i, id_field_offset), nullptr, 10) : id;
        const char* version = m_config.m_postgres_config.metadata.version() ? PQgetvalue(result, i, m_version_index) : nullptr;
        const char* uid = m_config.m_postgres_config.metadata.uid() ? PQgetvalue(result, i, m_uid_index) : nullptr;
        const char* timestamp = m_config.m_postgres_config.metadata.timestamp() ? PQgetvalue(result, i, m_last_modified_index) : nullptr;
        const char* changeset = m_config.m_postgres_config.metadata.changeset() ? PQgetvalue(result, i, m_changeset_index) : nullptr;
        // get nodes of this relation
        char* param_values[1];
        char param[25];
        param_values[0] = param;
        sprintf(param, "%ld", osm_id);
        PGresult* result_members = m_node_relations_table.run_prepared_statement("get_relation_members", 1, param_values);
        std::vector<osm_vector_tile_impl::MemberIdRoleTypePos> members;
        int tuples = PQntuples(result_members);
        for (int j = 0; j < tuples; ++j) {
            osmium::object_id_type ref = strtoll(PQgetvalue(result_members, j, 0), nullptr, 10);
            std::string role = PQgetvalue(result_members, j, 1);
            int pos = atoi(PQgetvalue(result_members, j, 2));
            members.emplace_back(ref, osmium::item_type::node, std::move(role), pos);
        }
        PQclear(result_members);
        result_members = m_way_relations_table.run_prepared_statement("get_relation_members", 1, param_values);
        tuples = PQntuples(result_members);
        for (int j = 0; j < tuples; ++j) {
            osmium::object_id_type ref = strtoll(PQgetvalue(result_members, j, 0), nullptr, 10);
            std::string role = PQgetvalue(result_members, j, 1);
            int pos = atoi(PQgetvalue(result_members, j, 2));
            members.emplace_back(ref, osmium::item_type::way, std::move(role), pos);
        }
        PQclear(result_members);
        result_members = m_relation_relations_table.run_prepared_statement("get_relation_members", 1, param_values);
        tuples = PQntuples(result_members);
        for (int j = 0; j < tuples; ++j) {
            osmium::object_id_type ref = strtoll(PQgetvalue(result_members, j, 0), nullptr, 10);
            std::string role = PQgetvalue(result_members, j, 1);
            int pos = atoi(PQgetvalue(result_members, j, 2));
            members.emplace_back(ref, osmium::item_type::relation, std::move(role), pos);
        }
        std::sort(members.begin(), members.end());
        m_add_relation_callback(osm_id, std::move(members), version, changeset, uid, timestamp,
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
    int id_field_offset = m_metadata_field_count;
    int tags_field_offset = m_metadata_field_count;
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
        const char* version = m_config.m_postgres_config.metadata.version() ? PQgetvalue(result, i, m_version_index) : nullptr;
        const char* uid = m_config.m_postgres_config.metadata.uid() ? PQgetvalue(result, i, m_uid_index) : nullptr;
        const char* timestamp = m_config.m_postgres_config.metadata.timestamp() ? PQgetvalue(result, i, m_last_modified_index) : nullptr;
        const char* changeset = m_config.m_postgres_config.metadata.changeset() ? PQgetvalue(result, i, m_changeset_index) : nullptr;
        if (with_tags) {
            std::string tags_hstore = PQgetvalue(result, i, tags_field_offset);
            m_add_node_callback(osm_id, location, version, changeset, uid, timestamp, std::move(tags_hstore));
        } else {
            m_add_node_callback(osm_id, location, version, changeset, uid, timestamp, "");
        }
    }
}

void CerepsoDataAccess::parse_way_query_result(PGresult* result, const osmium::object_id_type id) {
    int id_field_offset = m_metadata_field_count;
    int tags_field_offset = id_field_offset;
    if (id == 0) {
        ++tags_field_offset;
    }
    int tuple_count = PQntuples(result);
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        std::string tags_hstore = PQgetvalue(result, i, tags_field_offset);
        osmium::object_id_type way_id = id;
        if (id == 0) {
            way_id = strtoll(PQgetvalue(result, i, id_field_offset), nullptr, 10);
        }
        const char* version = m_config.m_postgres_config.metadata.version() ? PQgetvalue(result, i, m_version_index) : nullptr;
        const char* uid = m_config.m_postgres_config.metadata.uid() ? PQgetvalue(result, i, m_uid_index) : nullptr;
        const char* timestamp = m_config.m_postgres_config.metadata.timestamp() ? PQgetvalue(result, i, m_last_modified_index) : nullptr;
        const char* changeset = m_config.m_postgres_config.metadata.changeset() ? PQgetvalue(result, i, m_changeset_index) : nullptr;

        // get nodes of this way
        char* param_values[1];
        char param[25];
        param_values[0] = param;
        sprintf(param, "%ld", way_id);
        PGresult* result_member_nodes = m_node_ways_table.run_prepared_statement("get_way_nodes", 1, param_values);
        std::vector<postgres_drivers::MemberIdPos> node_ids;
        int node_ways_tuples = PQntuples(result_member_nodes);
        for (int j = 0; j < node_ways_tuples; ++j) {
            node_ids.emplace_back(strtoll(PQgetvalue(result_member_nodes, j, 0), nullptr, 10), atoi(PQgetvalue(result_member_nodes, j, 1)));
        }
        std::sort(node_ids.begin(), node_ids.end());
        PQclear(result_member_nodes);
        m_add_way_callback(way_id, std::move(node_ids), version, changeset, uid, timestamp, std::move(tags_hstore));
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


