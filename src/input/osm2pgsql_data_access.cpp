/*
 * data_access.cpp
 *
 *  Created on:  2019-10-14
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "osm2pgsql_data_access.hpp"
#include "nodes_provider_factory.hpp"
#include <array_parser.hpp>
#include <osmium/osm/types_from_string.hpp>

postgres_drivers::Column input::Osm2pgsqlDataAccess::osm_id {"osm_id", postgres_drivers::ColumnType::BIGINT, postgres_drivers::ColumnClass::OSM_ID};
postgres_drivers::Column input::Osm2pgsqlDataAccess::tags {"tags", postgres_drivers::ColumnType::HSTORE, postgres_drivers::ColumnClass::TAGS_OTHER};
postgres_drivers::Column input::Osm2pgsqlDataAccess::point_column {"way", postgres_drivers::ColumnType::POINT, postgres_drivers::ColumnClass::GEOMETRY};
postgres_drivers::Column input::Osm2pgsqlDataAccess::way_column {"way", postgres_drivers::ColumnType::GEOMETRY, postgres_drivers::ColumnClass::GEOMETRY};
postgres_drivers::Column input::Osm2pgsqlDataAccess::nodes {"nodes", postgres_drivers::ColumnType::BIGINT_ARRAY, postgres_drivers::ColumnClass::WAY_NODES};
postgres_drivers::Column input::Osm2pgsqlDataAccess::members {"members", postgres_drivers::ColumnType::TEXT_ARRAY, postgres_drivers::ColumnClass::RELATION_TYPE_ID_ROLE};

OSMDataTable input::Osm2pgsqlDataAccess::build_table(const char* name,
        VectortileGeneratorConfig& config, postgres_drivers::ColumnsVector&& core_columns,
        postgres_drivers::ColumnsVector* additional_columns) {
    postgres_drivers::Columns cols {std::move(core_columns), postgres_drivers::TableType::OTHER};
    if (additional_columns) {
        for (auto& c : *additional_columns) {
            cols.push_back(c);
        }
    }
    return OSMDataTable{name, config.m_postgres_config, std::move(cols)};
}

input::Osm2pgsqlDataAccess::Osm2pgsqlDataAccess(VectortileGeneratorConfig& config,
        input::ColumnConfigParser& column_config_parser) :
    m_nodes_provider(),
    m_line_table(build_table("planet_osm_line", config, {osm_id, tags, way_column}, &(column_config_parser.line_columns()))),
    m_ways_table(build_table("planet_osm_ways", config, {osm_id, nodes})),
    m_polygon_table(build_table("planet_osm_polygon", config, {osm_id, tags, way_column}, &(column_config_parser.polygon_columns()))),
    m_rels_table(build_table("planet_osm_rels", config, {osm_id, members})) {
    postgres_drivers::Columns point_columns {{osm_id, tags, point_column}, postgres_drivers::TableType::OTHER};
    point_columns.insert(column_config_parser.point_columns());
    OSMDataTable point_table {"planet_osm_point", config.m_postgres_config, std::move(point_columns)};
    m_nodes_provider = input::NodesProviderFactory::flatnodes_provider(config, column_config_parser, std::move(point_table));
    create_prepared_statements();
}

input::Osm2pgsqlDataAccess::Osm2pgsqlDataAccess(Osm2pgsqlDataAccess&& other) :
    m_nodes_provider(std::move(other.m_nodes_provider)),
    m_line_table(std::move(other.m_line_table)),
    m_ways_table(std::move(other.m_ways_table)),
    m_polygon_table(std::move(other.m_polygon_table)),
    m_rels_table(std::move(other.m_rels_table)),
    m_add_way_callback(other.m_add_way_callback),
    m_add_relation_callback(other.m_add_relation_callback) {
}

void input::Osm2pgsqlDataAccess::create_prepared_statements() {
    // ways
    std::string query = "SELECT osm_id FROM %1% WHERE ST_INTERSECTS(way, ST_MakeEnvelope($1, $2, $3, $4, 4326)) AND osm_id > 0";
    query = (boost::format(query) % m_line_table.get_name()).str();
    m_line_table.create_prepared_statement("get_lines", query, 4);

    query = "SELECT osm_id FROM %1% WHERE ST_INTERSECTS(way, ST_MakeEnvelope($1, $2, $3, $4, 4326)) AND osm_id > 0";
    query = (boost::format(query) % m_polygon_table.get_name()).str();
    m_polygon_table.create_prepared_statement("get_way_polygons", query, 4);

    query = "SELECT -osm_id AS osm_id FROM %1% WHERE ST_INTERSECTS(way, ST_MakeEnvelope($1, $2, $3, $4, 4326)) AND osm_id < 0";
    query = (boost::format(query) % m_polygon_table.get_name()).str();
    m_polygon_table.create_prepared_statement("get_relation_polygons", query, 4);

    // way nodes and tags
    query = (boost::format("SELECT nodes, tags FROM %1% WHERE id = $1")
        % m_ways_table.get_name()).str();
    m_ways_table.create_prepared_statement("get_way_nodes_and_tags", query, 1);

    // relation members and tags
    query = (boost::format("SELECT members, tags FROM %1% WHERE id = $1")
        % m_rels_table.get_name()).str();
    m_rels_table.create_prepared_statement("get_relation_members_and_tags", query, 1);
}

void input::Osm2pgsqlDataAccess::set_bbox(const BoundingBox& bbox) {
    m_nodes_provider->set_bbox(bbox);
    m_line_table.set_bbox(bbox);
    m_polygon_table.set_bbox(bbox);
}

void input::Osm2pgsqlDataAccess::set_add_node_callback(osm_vector_tile_impl::node_callback_type&& callback,
        osm_vector_tile_impl::node_without_tags_callback_type&& callback_without_tags,
        osm_vector_tile_impl::simple_node_callback_type&& simple_callback) {
    m_nodes_provider->set_add_node_callback(callback);
    m_nodes_provider->set_add_node_without_tags_callback(callback_without_tags);
    m_nodes_provider->set_add_simple_node_callback(simple_callback);
}

void input::Osm2pgsqlDataAccess::set_add_way_callback(osm_vector_tile_impl::way_callback_type&& /*callback*/,
        osm_vector_tile_impl::slim_way_callback_type&& slim_callback) {
    m_add_way_callback = slim_callback;
}

void input::Osm2pgsqlDataAccess::set_add_relation_callback(osm_vector_tile_impl::relation_callback_type&& /*callback*/,
        osm_vector_tile_impl::slim_relation_callback_type&& slim_callback) {
    m_add_relation_callback = slim_callback;
}

void input::Osm2pgsqlDataAccess::get_nodes_inside() {
    m_nodes_provider->get_nodes_inside();
}

void input::Osm2pgsqlDataAccess::get_missing_nodes(const osm_vector_tile_impl::osm_id_set_type& missing_nodes) {
    m_nodes_provider->get_missing_nodes(missing_nodes);
}

void input::Osm2pgsqlDataAccess::parse_way_query_result(PGresult* result) {
    int tuple_count = PQntuples(result);
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        osmium::object_id_type way_id = strtoll(PQgetvalue(result, i, 0), nullptr, 10);
        way_nodes_and_tags(way_id);
    }
}

void input::Osm2pgsqlDataAccess::throw_db_related_exception(const char* templ,
        const char* table_name, const osmium::object_id_type id) {
    constexpr unsigned long int len_msg = 255;
    std::unique_ptr<char> msg {new char[len_msg]};
    snprintf(msg.get(), len_msg - 1, templ, table_name, id);
    throw std::runtime_error{msg.get()};
}

void input::Osm2pgsqlDataAccess::way_nodes_and_tags(osmium::object_id_type id) {
    char* param_values[1];
    char param[25];
    sprintf(param, "%ld", id);
    param_values[0] = param;
    PGresult* result = m_ways_table.run_prepared_statement("get_way_nodes_and_tags", 1, param_values);
    std::vector<postgres_drivers::MemberIdPos> node_ids;
    if (PQntuples(result) != 1) {
        PQclear(result);
        throw_db_related_exception("Database is in inconsistent state. There are more than one entries in %s table for way %ld.",
                m_ways_table.get_name().c_str(), id);
    }
    std::string nodes_arr_str = PQgetvalue(result, 0, 0);
    if (nodes_arr_str.empty()) {
        PQclear(result);
        throw_db_related_exception("Database is in inconsistent state. There are no nodes in %s table for way %ld.",
                        m_ways_table.get_name().c_str(), id);
    }
    pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::Int64Conversion> nodes_array_parser(nodes_arr_str);
    size_t pos = 0;
    while (nodes_array_parser.has_next()) {
        node_ids.emplace_back(nodes_array_parser.get_next(), pos);
        ++pos;
    }

    std::string tags_arr_str = PQgetvalue(result, 0, 1);
    std::vector<osm_vector_tile_impl::StringPair> tags = tags_from_pg_string_array(tags_arr_str);
    m_add_way_callback(id, std::move(node_ids), nullptr, nullptr, nullptr, nullptr,
            tags);
    PQclear(result);
}

void input::Osm2pgsqlDataAccess::get_ways(OSMDataTable& table, const char* prepared_statement_name) {
    PGresult* result = table.run_prepared_bbox_statement(prepared_statement_name);
    parse_way_query_result(result);
    PQclear(result);
}

void input::Osm2pgsqlDataAccess::get_ways_inside() {
    get_ways(m_line_table, "get_lines");
    get_ways(m_polygon_table, "get_way_polygons");
}

void input::Osm2pgsqlDataAccess::get_missing_ways(const osm_vector_tile_impl::osm_id_set_type& missing_ways) {
    for (const osmium::object_id_type id : missing_ways) {
        way_nodes_and_tags(id);
    }
}

void input::Osm2pgsqlDataAccess::parse_relation_query_result(PGresult* result) {
    int tuple_count = PQntuples(result);
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        osmium::object_id_type relation_id = strtoll(PQgetvalue(result, i, 0), nullptr, 10);
        relation_tags_and_members(relation_id);
    }
}

std::vector<osm_vector_tile_impl::StringPair> input::Osm2pgsqlDataAccess::tags_from_pg_string_array(std::string& tags_arr_str) {
    std::vector<osm_vector_tile_impl::StringPair> tags;
    if (tags_arr_str.empty()) {
        return tags;
    }
    pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> tags_array_parser(tags_arr_str);
    size_t pos = 0;
    std::string key;
    while (tags_array_parser.has_next()) {
        if (pos % 2 == 0) {
            key = std::move(tags_array_parser.get_next());
        } else {
            tags.emplace_back(std::move(key), std::move(tags_array_parser.get_next()));
        }
        ++pos;
    }
    return tags;
}

void input::Osm2pgsqlDataAccess::relation_tags_and_members(const osmium::object_id_type id) {
    char* param_values[1];
    char param[25];
    sprintf(param, "%ld", id);
    param_values[0] = param;
    PGresult* result = m_rels_table.run_prepared_statement("get_relation_members_and_tags", 1, param_values);
    if (PQntuples(result) == 0) {
        return;
    }
    std::vector<osm_vector_tile_impl::MemberIdRoleTypePos> members;
    if (PQntuples(result) != 1) {
        PQclear(result);
        throw_db_related_exception("Database is in inconsistent state. There are no entries in %s table for relation %ld.",
                        m_ways_table.get_name().c_str(), id);
    }
    std::string members_arr_str = PQgetvalue(result, 0, 0);
    std::pair<osmium::item_type, osmium::object_id_type> id_type;
    if (!members_arr_str.empty()) {
        // a relation without nodes is valid with API 0.6
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> array_parser(members_arr_str);
        size_t item_count = 0;
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
    }

    std::string tags_arr_str = PQgetvalue(result, 0, 1);
    std::vector<osm_vector_tile_impl::StringPair> tags = tags_from_pg_string_array(tags_arr_str);
    m_add_relation_callback(id, std::move(members), nullptr, nullptr, nullptr, nullptr, tags);
    PQclear(result);
}

void input::Osm2pgsqlDataAccess::get_relations_inside() {
    PGresult* result = m_polygon_table.run_prepared_bbox_statement("get_relation_polygons");
    parse_relation_query_result(result);
    PQclear(result);
}

void input::Osm2pgsqlDataAccess::get_missing_relations(const osm_vector_tile_impl::osm_id_set_type& missing_relations) {
    for (const osmium::object_id_type id : missing_relations) {
        // TODO query all relations in one go
        relation_tags_and_members(id);
    }
}
