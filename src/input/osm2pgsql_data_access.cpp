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

std::vector<osmium::object_id_type> input::Osm2pgsqlDataAccess::get_ids_inside(
        OSMDataTable& table, const char* prepared_statement_name) {
    std::vector<osmium::object_id_type> ids;
    PGresult* result = table.run_prepared_bbox_statement(prepared_statement_name);
    int tuple_count = PQntuples(result);
    ids.reserve(tuple_count);
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        ids.push_back(strtoll(PQgetvalue(result, i, 0), nullptr, 10));
    }
    PQclear(result);
    return ids;
}

void input::Osm2pgsqlDataAccess::throw_db_related_exception(const char* templ,
        const char* table_name, const osmium::object_id_type id) {
    constexpr unsigned long int len_msg = 255;
    std::unique_ptr<char> msg {new char[len_msg]};
    snprintf(msg.get(), len_msg - 1, templ, table_name, id);
    throw std::runtime_error{msg.get()};
}

void input::Osm2pgsqlDataAccess::query_and_flush_ways(char* sql_query) {
    PGresult* result = m_ways_table.send_select_query(sql_query);
    int row_count = PQntuples(result);
    if (row_count == 0) {
        return;
    }
    for (int i = 0; i < row_count; ++i) {
        osmium::object_id_type id = strtoll(PQgetvalue(result, i, 0), nullptr, 10);
        std::vector<postgres_drivers::MemberIdPos> node_ids;
        std::string nodes_arr_str = PQgetvalue(result, i, 1);
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

        std::string tags_arr_str = PQgetvalue(result, i, 2);
        std::vector<osm_vector_tile_impl::StringPair> tags = tags_from_pg_string_array(tags_arr_str);
        m_add_way_callback(id, std::move(node_ids), nullptr, nullptr, nullptr, nullptr,
                tags);
    }
    PQclear(result);
}

void input::Osm2pgsqlDataAccess::get_ways(OSMDataTable& table, const char* prepared_statement_name) {
    std::vector<osmium::object_id_type> ids = get_ids_inside(table, prepared_statement_name);
    get_objects_from_db<std::vector<osmium::object_id_type>::iterator>(ids.begin(), ids.end(),
        "id, nodes, tags", m_ways_table.get_name().c_str(),
        [this](char* sql_query) {
            this->query_and_flush_ways(sql_query);
        }
    );
}

void input::Osm2pgsqlDataAccess::get_ways_inside() {
    get_ways(m_line_table, "get_lines");
    get_ways(m_polygon_table, "get_way_polygons");
}

void input::Osm2pgsqlDataAccess::get_missing_ways(const osm_vector_tile_impl::osm_id_set_type& missing_ways) {
    using iterator_type = osm_vector_tile_impl::osm_id_set_type::const_iterator;
    get_objects_from_db<iterator_type>(missing_ways.begin(), missing_ways.end(),
        "id, nodes, tags", m_ways_table.get_name().c_str(),
        [this](char* sql_query) {
            this->query_and_flush_ways(sql_query);
        }
    );
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

void input::Osm2pgsqlDataAccess::query_and_flush_relations(char* sql_query) {
    PGresult* result = m_rels_table.send_select_query(sql_query);
    int row_count = PQntuples(result);
    if (row_count == 0) {
        PQclear(result);
        return;
    }
    for (int i = 0; i < row_count; ++i) {
        osmium::object_id_type id = strtoll(PQgetvalue(result, i, 0), nullptr, 10);
        std::vector<osm_vector_tile_impl::MemberIdRoleTypePos> members;
        std::string members_arr_str = PQgetvalue(result, i, 1);
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

        std::string tags_arr_str = PQgetvalue(result, i, 2);
        std::vector<osm_vector_tile_impl::StringPair> tags = tags_from_pg_string_array(tags_arr_str);
        m_add_relation_callback(id, std::move(members), nullptr, nullptr, nullptr, nullptr, tags);
    }
    PQclear(result);
}

void input::Osm2pgsqlDataAccess::get_relations_inside() {
    std::vector<osmium::object_id_type> ids = get_ids_inside(m_polygon_table, "get_relation_polygons");
    get_objects_from_db<std::vector<osmium::object_id_type>::iterator>(ids.begin(), ids.end(),
        "id, members, tags", m_rels_table.get_name().c_str(),
        [this](char* sql_query) {
            this->query_and_flush_relations(sql_query);
        }
    );
}

void input::Osm2pgsqlDataAccess::get_missing_relations(const osm_vector_tile_impl::osm_id_set_type& missing_relations) {
    using iterator_type = osm_vector_tile_impl::osm_id_set_type::const_iterator;
    get_objects_from_db<iterator_type>(missing_relations.begin(), missing_relations.end(),
        "id, members, tags", m_rels_table.get_name().c_str(),
        [this](char* sql_query) {
            this->query_and_flush_relations(sql_query);
        }
    );
}
