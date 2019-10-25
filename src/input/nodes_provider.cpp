/*
 * untagged_nodes_provider.cpp
 *
 *  Created on:  2019-10-09
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "nodes_provider.hpp"
#include <string>


input::NodesProvider::NodesProvider(VectortileGeneratorConfig& config,
        ColumnConfigParser& column_config_parser, OSMDataTable&& nodes_table) :
    m_config(config),
    m_column_config_parser(column_config_parser),
    m_nodes_table(std::move(nodes_table)),
    m_metadata(config) {
    create_prepared_statements();
}

input::NodesProvider::~NodesProvider() {
}

void input::NodesProvider::set_add_node_callback(osm_vector_tile_impl::node_callback_type& callback) {
    m_add_node_callback = callback;
}

void input::NodesProvider::set_add_node_without_tags_callback(osm_vector_tile_impl::node_without_tags_callback_type& callback) {
    m_add_node_without_tags_callback = callback;
}

void input::NodesProvider::set_add_simple_node_callback(osm_vector_tile_impl::simple_node_callback_type& callback) {
    m_add_simple_node_callback = callback;
}

void input::NodesProvider::set_bbox(const BoundingBox& bbox) {
    m_nodes_table.set_bbox(bbox);
}

void input::NodesProvider::create_prepared_statements() {
    std::string geom_column_name = m_nodes_table.get_column_name_by_type(postgres_drivers::ColumnType::POINT);
    std::string columns = postgres_drivers::join_columns_to_str(m_column_config_parser.point_columns(), true);
    std::string query = m_metadata.select_str();
    query += " osm_id, tags, ST_X(%1%), ST_Y(%1%) %2% FROM %3% WHERE ST_INTERSECTS(%1%, ST_MakeEnvelope($1, $2, $3, $4, 4326))";
    query = (boost::format(query) % geom_column_name % columns % m_nodes_table.get_name()).str();
    m_nodes_table.create_prepared_statement("get_nodes_with_tags", query, 4);

    query = m_metadata.select_str();
    query += " tags, ST_X(%1%), ST_Y(%1%) %2%";
    query.append(" FROM %3% WHERE osm_id = $1");
    query = (boost::format(query) % geom_column_name % columns % m_nodes_table.get_name()).str();
    m_nodes_table.create_prepared_statement("get_single_node_with_tags", query, 1);
}

void input::NodesProvider::get_nodes_inside() {
    PGresult* result = m_nodes_table.run_prepared_bbox_statement("get_nodes_with_tags");
    parse_node_query_result(result, true, 0);
    PQclear(result);
}

bool input::NodesProvider::get_node_with_tags(const osmium::object_id_type id, char** param_values) {
    PGresult* result = m_nodes_table.run_prepared_statement("get_single_node_with_tags", 1, param_values);
    if (PQntuples(result) > 0) {
        parse_node_query_result(result, true, id);
        PQclear(result);
        return true;
    }
    PQclear(result);
    return false;
}

/*static*/ void input::NodesProvider::parse_node_query_result(PGresult* result, const bool with_tags,
        const osmium::object_id_type id) {
    int id_field_offset = m_metadata.count();
    int tags_field_offset = m_metadata.count();
    if (id == 0) {
        ++tags_field_offset;
    }
    int other_field_offset = with_tags ? tags_field_offset + 1 : tags_field_offset;
    int additional_values_offset = other_field_offset + 2;
    int tuple_count = PQntuples(result);
    postgres_drivers::ColumnsVector& point_columns = m_column_config_parser.point_columns();
    std::vector<const char*> additional_values {point_columns.size(), nullptr};
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
        for (size_t j = 0; j < point_columns.size(); ++j) {
            additional_values[j] = PQgetvalue(result, i, additional_values_offset + j);
        }
        if (with_tags) {
            std::string tags_hstore = PQgetvalue(result, i, tags_field_offset);
            m_add_node_callback(osm_id, location, version, changeset, uid, timestamp,
                    std::move(tags_hstore), point_columns, additional_values);
        } else {
            m_add_node_without_tags_callback(osm_id, location, version, changeset, uid, timestamp);
        }
    }
}
