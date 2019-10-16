/*
 * data_access.cpp
 *
 *  Created on:  2019-10-14
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "data_access.hpp"
#include "../cerepso/nodes_provider_factory.hpp"
#include <array_parser.hpp>
#include <osmium/osm/types_from_string.hpp>

postgres_drivers::Column input::osm2pgsql::DataAccess::osm_id {"osm_id", postgres_drivers::ColumnType::BIGINT, postgres_drivers::ColumnClass::OSM_ID};
postgres_drivers::Column input::osm2pgsql::DataAccess::tags {"tags", postgres_drivers::ColumnType::HSTORE, postgres_drivers::ColumnClass::TAGS_OTHER};
postgres_drivers::Column input::osm2pgsql::DataAccess::nodes {"nodes", postgres_drivers::ColumnType::BIGINT_ARRAY, postgres_drivers::ColumnClass::WAY_NODES};
postgres_drivers::Column input::osm2pgsql::DataAccess::members {"members", postgres_drivers::ColumnType::TEXT_ARRAY, postgres_drivers::ColumnClass::RELATION_TYPE_ID_ROLE};

input::osm2pgsql::DataAccess::DataAccess(VectortileGeneratorConfig& config) :
    m_nodes_provider(input::cerepso::NodesProviderFactory::flatnodes_provider(config, "planet_osm_point")),
    m_line_table("planet_osm_line", config.m_postgres_config, postgres_drivers::Columns({osm_id, tags}, postgres_drivers::TableType::OTHER)),
    m_ways_table("planet_osm_ways", config.m_postgres_config, postgres_drivers::Columns({osm_id, tags}, postgres_drivers::TableType::OTHER)),
    m_polygon_table("planet_osm_polygon", config.m_postgres_config, postgres_drivers::Columns({nodes}, postgres_drivers::TableType::OTHER)),
    m_rels_table("planet_osm_rels", config.m_postgres_config, postgres_drivers::Columns({members}, postgres_drivers::TableType::OTHER)) {
    postgres_drivers::Columns rels_columns {{members}, postgres_drivers::TableType::OTHER};
    create_prepared_statements();
}

input::osm2pgsql::DataAccess::DataAccess(DataAccess&& other) :
    m_nodes_provider(std::move(other.m_nodes_provider)),
    m_line_table(std::move(other.m_line_table)),
    m_ways_table(std::move(other.m_ways_table)),
    m_polygon_table(std::move(other.m_polygon_table)),
    m_rels_table(std::move(other.m_rels_table)) {
}

void input::osm2pgsql::DataAccess::create_prepared_statements() {
    // ways
    std::string query = "SELECT osm_id, tags FROM %1% WHERE ST_INTERSECTS(way, ST_MakeEnvelope($1, $2, $3, $4, 4326)) AND osm_id > 0";
    query = (boost::format(query) % m_line_table.get_name()).str();
    m_line_table.create_prepared_statement("get_lines", query, 4);

    query = "SELECT tags FROM %1% WHERE osm_id = $1";
    query = (boost::format(query) % m_line_table.get_name()).str();
    m_line_table.create_prepared_statement("get_single_line", query, 1);

    query = "SELECT osm_id, tags FROM %1% WHERE ST_INTERSECTS(way, ST_MakeEnvelope($1, $2, $3, $4, 4326)) AND osm_id > 0";
    query = (boost::format(query) % m_polygon_table.get_name()).str();
    m_polygon_table.create_prepared_statement("get_way_polygons", query, 4);

    query = "SELECT -osm_id AS osm_id, tags FROM %1% WHERE ST_INTERSECTS(way, ST_MakeEnvelope($1, $2, $3, $4, 4326)) AND osm_id < 0";
    query = (boost::format(query) % m_polygon_table.get_name()).str();
    m_polygon_table.create_prepared_statement("get_relation_polygons", query, 4);

    query = "SELECT tags FROM %1% WHERE osm_id = $1";
    query = (boost::format(query) % m_polygon_table.get_name()).str();
    m_polygon_table.create_prepared_statement("get_single_way_polygon", query, 1);

    query = "SELECT tags FROM %1% WHERE osm_id = (-1::bigint * $1)";
    query = (boost::format(query) % m_polygon_table.get_name()).str();
    m_polygon_table.create_prepared_statement("get_single_relation_polygon", query, 1);

    // way nodes
    query = (boost::format("SELECT nodes FROM %1% WHERE id = $1")
        % m_ways_table.get_name()).str();
    m_ways_table.create_prepared_statement("get_way_nodes", query, 1);

    // relation members
    query = (boost::format("SELECT members FROM %1% WHERE id = $1")
        % m_rels_table.get_name()).str();
    m_rels_table.create_prepared_statement("get_relation_members", query, 1);
}

void input::osm2pgsql::DataAccess::set_bbox(const BoundingBox& bbox) {
    m_nodes_provider->set_bbox(bbox);
    m_line_table.set_bbox(bbox);
    m_polygon_table.set_bbox(bbox);
}

void input::osm2pgsql::DataAccess::set_add_node_callback(osm_vector_tile_impl::node_callback_type&& callback,
        osm_vector_tile_impl::simple_node_callback_type&& simple_callback) {
    m_add_node_callback = callback;
    m_nodes_provider->set_add_node_callback(callback);
    m_nodes_provider->set_add_simple_node_callback(simple_callback);
}

void input::osm2pgsql::DataAccess::set_add_way_callback(osm_vector_tile_impl::way_callback_type&& callback) {
    m_add_way_callback = callback;
}

void input::osm2pgsql::DataAccess::set_add_relation_callback(osm_vector_tile_impl::relation_callback_type&& callback) {
    m_add_relation_callback = callback;
}

void input::osm2pgsql::DataAccess::get_nodes_inside() {
    m_nodes_provider->get_nodes_inside();
}

void input::osm2pgsql::DataAccess::get_missing_nodes(const osm_vector_tile_impl::osm_id_set_type& missing_nodes) {
    m_nodes_provider->get_missing_nodes(missing_nodes);
}

void input::osm2pgsql::DataAccess::parse_way_query_result(PGresult* result, osmium::object_id_type id) {
    int tuple_count = PQntuples(result);
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        std::string tags_hstore;
        osmium::object_id_type way_id = id;
        if (id == 0) {
            way_id = strtoll(PQgetvalue(result, i, 0), nullptr, 10);
            tags_hstore = PQgetvalue(result, i, 1);
        } else {
            tags_hstore = PQgetvalue(result, i, 0);
        }

        // get nodes of this way
        char* param_values[1];
        char param[25];
        param_values[0] = param;
        sprintf(param, "%ld", way_id);
        PGresult* result_member_nodes = m_ways_table.run_prepared_statement("get_way_nodes", 1, param_values);
        std::vector<postgres_drivers::MemberIdPos> node_ids;
        if (PQntuples(result_member_nodes) != 1) {
            std::string msg = "Database is in inconsistent state. Way ";
            msg += std::to_string(way_id);
            msg += " has no or more than one entries in ";
            msg += m_ways_table.get_name();
            msg += " table.";
            PQclear(result_member_nodes);
            PQclear(result);
            throw std::runtime_error{msg};
        }
        std::string nodes_arr_str = PQgetvalue(result_member_nodes, 0, 0);
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::Int64Conversion> array_parser(nodes_arr_str);
        size_t pos = 0;
        while (array_parser.has_next()) {
            node_ids.emplace_back(array_parser.get_next(), pos);
            ++pos;
        }
        PQclear(result_member_nodes);
        m_add_way_callback(way_id, std::move(node_ids), nullptr, nullptr, nullptr, nullptr, std::move(tags_hstore));
    }
}

void input::osm2pgsql::DataAccess::get_ways(OSMDataTable& table, const char* prepared_statement_name) {
    PGresult* result = table.run_prepared_bbox_statement(prepared_statement_name);
    parse_way_query_result(result, 0);
    PQclear(result);
}

void input::osm2pgsql::DataAccess::get_ways_inside() {
    get_ways(m_line_table, "get_lines");
    get_ways(m_polygon_table, "get_way_polygons");
}

void input::osm2pgsql::DataAccess::get_missing_ways(const osm_vector_tile_impl::osm_id_set_type& missing_ways) {
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

void input::osm2pgsql::DataAccess::parse_relation_query_result(PGresult* result, osmium::object_id_type id) {
    int tuple_count = PQntuples(result);
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        std::string tags_hstore;
        osmium::object_id_type way_id = id;
        if (id == 0) {
            way_id = strtoll(PQgetvalue(result, i, 0), nullptr, 10);
            tags_hstore = PQgetvalue(result, i, 1);
        } else {
            tags_hstore = PQgetvalue(result, i, 0);
        }

        // get nodes of this way
        char* param_values[1];
        char param[25];
        param_values[0] = param;
        sprintf(param, "%ld", way_id);
        PGresult* result_member_nodes = m_rels_table.run_prepared_statement("get_relation_members", 1, param_values);
        std::vector<osm_vector_tile_impl::MemberIdRoleTypePos> members;
        if (PQntuples(result_member_nodes) != 1) {
            std::string msg = "Database is in inconsistent state. Relation ";
            msg += std::to_string(way_id);
            msg += " has no or more than one entries in ";
            msg += m_rels_table.get_name();
            msg += " table.";
            PQclear(result_member_nodes);
            PQclear(result);
            throw std::runtime_error{msg};
        }
        std::string nodes_arr_str = PQgetvalue(result_member_nodes, 0, 0);
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> array_parser(nodes_arr_str);
        size_t item_count = 0;
        std::pair<osmium::item_type, osmium::object_id_type> id_type;
        while (array_parser.has_next()) {
            std::string item = array_parser.get_next();
            if (item_count % 2 == 0) {
                // split into type and ID
                id_type = osmium::string_to_object_id(item.c_str(), osmium::osm_entity_bits::nwr);
            } else {
                // current item is role
                members.emplace_back(id_type.second, id_type.first, std::move(item), item_count/2);
            }
            ++item_count;
        }
        PQclear(result_member_nodes);
        m_add_relation_callback(way_id, std::move(members), nullptr, nullptr, nullptr, nullptr, std::move(tags_hstore));
    }
}

void input::osm2pgsql::DataAccess::get_relations_inside() {
    PGresult* result = m_polygon_table.run_prepared_bbox_statement("get_relation_polygons");
    PQclear(result);
}

void input::osm2pgsql::DataAccess::get_missing_relations(const osm_vector_tile_impl::osm_id_set_type& missing_relations) {
    char* param_values[1];
    char param[25];
    param_values[0] = param;
    for (const osmium::object_id_type id : missing_relations) {
        sprintf(param, "%ld", id);
        PGresult* result = m_polygon_table.run_prepared_statement("get_single_relation_polygon", 1, param_values);
        parse_relation_query_result(result, id);
        PQclear(result);
    }
}
