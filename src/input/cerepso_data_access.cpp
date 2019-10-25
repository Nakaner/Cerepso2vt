/*
 * cerepso_data_access.cpp
 *
 *  Created on:  2019-08-19
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "cerepso_data_access.hpp"
#include "nodes_provider_factory.hpp"
#include <algorithm>

OSMDataTable input::CerepsoDataAccess::build_table(const char* name,
        VectortileGeneratorConfig& config, postgres_drivers::TableType type,
        postgres_drivers::ColumnsVector* additional_columns = nullptr) {
    postgres_drivers::Columns cols {config.m_postgres_config, type};
    if (additional_columns) {
        for (auto& c : *additional_columns) {
            cols.push_back(c);
        }
    }
    return OSMDataTable{name, config.m_postgres_config, std::move(cols)};
}

input::CerepsoDataAccess::CerepsoDataAccess(VectortileGeneratorConfig& config,
        input::ColumnConfigParser& column_config_parser) :
    m_column_config_parser(column_config_parser),
    m_config(config),
    m_ways_table(build_table("planet_osm_line", config, postgres_drivers::TableType::WAYS_LINEAR, &(column_config_parser.line_columns()))),
    m_relations_table(build_table("relations", config, postgres_drivers::TableType::RELATION_OTHER, &(column_config_parser.line_columns()))),
    m_node_ways_table(build_table("node_ways", config, postgres_drivers::TableType::NODE_WAYS)),
    m_node_relations_table(build_table("node_relations", config, postgres_drivers::TableType::RELATION_MEMBER_NODES)),
    m_way_relations_table(build_table("way_relations", config, postgres_drivers::TableType::RELATION_MEMBER_WAYS)),
    m_relation_relations_table(build_table("relation_relations", config, postgres_drivers::TableType::RELATION_MEMBER_RELATIONS)),
    m_metadata_fields(config) {
    // initialize the implementaion used to produce the vector tile
    OSMDataTable nodes_table = build_table("planet_osm_point", config, postgres_drivers::TableType::POINT, &(column_config_parser.point_columns()));
    if (config.m_flatnodes_path == "") {
        OSMDataTable untagged_nodes_table {"untagged_nodes", config.m_postgres_config, {config.m_postgres_config, postgres_drivers::TableType::UNTAGGED_POINT}};
        m_nodes_provider = input::NodesProviderFactory::db_provider(config, column_config_parser, std::move(nodes_table), std::move(untagged_nodes_table));
    } else {
        m_nodes_provider = input::NodesProviderFactory::flatnodes_provider(config, column_config_parser, std::move(nodes_table));
    }
    create_prepared_statements();
}

input::CerepsoDataAccess::CerepsoDataAccess(CerepsoDataAccess&& other) :
    m_column_config_parser(other.m_column_config_parser),
    m_config(other.m_config),
    m_nodes_provider(std::move(other.m_nodes_provider)),
    m_ways_table(std::move(other.m_ways_table)),
    m_relations_table(std::move(other.m_relations_table)),
    m_node_ways_table(std::move(other.m_node_ways_table)),
    m_node_relations_table(std::move(other.m_node_relations_table)),
    m_way_relations_table(std::move(other.m_way_relations_table)),
    m_relation_relations_table(std::move(other.m_relation_relations_table)),
    m_metadata_fields(other.m_config),
    m_add_way_callback(other.m_add_way_callback),
    m_add_relation_callback(other.m_add_relation_callback) {
}

void input::CerepsoDataAccess::create_prepared_statements() {
    // ways
    std::string query = m_metadata_fields.select_str();
    query += " osm_id, tags";
    query.append(" FROM %1% WHERE ST_INTERSECTS(geom, ST_MakeEnvelope($1, $2, $3, $4, 4326))");
    query = (boost::format(query) % m_ways_table.get_name()).str();
    m_ways_table.create_prepared_statement("get_ways", query, 4);

    query = m_metadata_fields.select_str();
    query += " tags";
    query.append(" FROM %1% WHERE osm_id = $1");
    query = (boost::format(query) % m_ways_table.get_name()).str();
    m_ways_table.create_prepared_statement("get_single_way", query, 1);

    query = (boost::format("SELECT node_id, position FROM %1% WHERE way_id = $1")
        % m_node_ways_table.get_name()).str();
    m_node_ways_table.create_prepared_statement("get_way_nodes", query, 1);

    // relations
    query = m_metadata_fields.select_str();
    query += " osm_id, tags";
    query.append(" FROM %1% WHERE ST_INTERSECTS(geom_points, " \
            "ST_MakeEnvelope($1, $2, $3, $4, 4326)) OR ST_INTERSECTS(geom_lines, ST_MakeEnvelope($1, $2, $3, $4, 4326))");
    query = (boost::format(query) % m_relations_table.get_name()).str();
    m_relations_table.create_prepared_statement("get_relations", query, 4);

    query = m_metadata_fields.select_str();
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

void input::CerepsoDataAccess::set_bbox(const BoundingBox& bbox) {
    m_nodes_provider->set_bbox(bbox);
    m_ways_table.set_bbox(bbox);
    m_relations_table.set_bbox(bbox);
}

void input::CerepsoDataAccess::set_add_node_callback(osm_vector_tile_impl::node_callback_type&& callback,
        osm_vector_tile_impl::node_without_tags_callback_type&& callback_without_tags,
        osm_vector_tile_impl::simple_node_callback_type&& simple_callback) {
    m_nodes_provider->set_add_node_callback(callback);
    m_nodes_provider->set_add_node_without_tags_callback(callback_without_tags);
    m_nodes_provider->set_add_simple_node_callback(simple_callback);
}

void input::CerepsoDataAccess::set_add_way_callback(osm_vector_tile_impl::way_callback_type&& callback,
        osm_vector_tile_impl::slim_way_callback_type&&) {
    m_add_way_callback = callback;
}

void input::CerepsoDataAccess::set_add_relation_callback(osm_vector_tile_impl::relation_callback_type&& callback,
        osm_vector_tile_impl::slim_relation_callback_type&&) {
    m_add_relation_callback = callback;
}

void input::CerepsoDataAccess::parse_relation_query_result(PGresult* result, const osmium::object_id_type id) {
    int id_field_offset = m_metadata_fields.count();
    int tags_field_offset = id_field_offset;
    if (id == 0) {
        ++tags_field_offset;
    }
    int additional_values_offset = tags_field_offset + 1;
    int tuple_count = PQntuples(result);
    postgres_drivers::ColumnsVector& columns = m_column_config_parser.polygon_columns();
    std::vector<const char*> additional_values {columns.size(), nullptr};
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        std::string tags_hstore = PQgetvalue(result, i, tags_field_offset);
        const osmium::object_id_type osm_id = (id == 0) ? strtoll(PQgetvalue(result, i, id_field_offset), nullptr, 10) : id;
        const char* version = m_metadata_fields.has_version() ? PQgetvalue(result, i, m_metadata_fields.version()) : nullptr;
        const char* uid = m_metadata_fields.has_uid() ? PQgetvalue(result, i, m_metadata_fields.uid()) : nullptr;
        const char* timestamp = m_metadata_fields.has_last_modified() ? PQgetvalue(result, i, m_metadata_fields.last_modified()) : nullptr;
        const char* changeset = m_metadata_fields.has_changeset() ? PQgetvalue(result, i, m_metadata_fields.changeset()) : nullptr;
        for (size_t j = 0; j < columns.size(); ++j) {
            additional_values[j] = PQgetvalue(result, i, additional_values_offset + j);
        }
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
                std::move(tags_hstore), columns, additional_values);
    }
}

void input::CerepsoDataAccess::get_missing_relations(const osm_vector_tile_impl::osm_id_set_type& missing_relations) {
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

void input::CerepsoDataAccess::get_missing_nodes(const osm_vector_tile_impl::osm_id_set_type& missing_nodes) {
    m_nodes_provider->get_missing_nodes(missing_nodes);
}

void input::CerepsoDataAccess::get_nodes_inside() {
    m_nodes_provider->get_nodes_inside();
}

void input::CerepsoDataAccess::parse_way_query_result(PGresult* result, const osmium::object_id_type id) {
    int id_field_offset = m_metadata_fields.count();
    int tags_field_offset = id_field_offset;
    if (id == 0) {
        ++tags_field_offset;
    }
    int additional_values_offset = tags_field_offset + 1;
    int tuple_count = PQntuples(result);
    postgres_drivers::ColumnsVector& columns = m_column_config_parser.polygon_columns();
    std::vector<const char*> additional_values {columns.size(), nullptr};
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        std::string tags_hstore = PQgetvalue(result, i, tags_field_offset);
        osmium::object_id_type way_id = id;
        if (id == 0) {
            way_id = strtoll(PQgetvalue(result, i, id_field_offset), nullptr, 10);
        }
        const char* version = m_metadata_fields.has_version() ? PQgetvalue(result, i, m_metadata_fields.version()) : nullptr;
        const char* uid = m_metadata_fields.has_uid() ? PQgetvalue(result, i, m_metadata_fields.uid()) : nullptr;
        const char* timestamp = m_metadata_fields.has_last_modified() ? PQgetvalue(result, i, m_metadata_fields.last_modified()) : nullptr;
        const char* changeset = m_metadata_fields.has_changeset() ? PQgetvalue(result, i, m_metadata_fields.changeset()) : nullptr;
        for (size_t j = 0; j < columns.size(); ++j) {
            additional_values[j] = PQgetvalue(result, i, additional_values_offset + j);
        }

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
        m_add_way_callback(way_id, std::move(node_ids), version, changeset, uid, timestamp,
                std::move(tags_hstore), columns, additional_values);
    }
}

void input::CerepsoDataAccess::get_ways_inside() {
    PGresult* result = m_ways_table.run_prepared_bbox_statement("get_ways");
    parse_way_query_result(result, 0);
    PQclear(result);
}

void input::CerepsoDataAccess::get_missing_ways(const osm_vector_tile_impl::osm_id_set_type& missing_ways) {
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

void input::CerepsoDataAccess::get_relations_inside() {
    PGresult* result = m_relations_table.run_prepared_bbox_statement("get_relations");
    parse_relation_query_result(result, 0);
    PQclear(result);
}


