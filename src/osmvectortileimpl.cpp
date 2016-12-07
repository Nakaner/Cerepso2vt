/*
 * osmvectortileimpl.cpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */

#include <osmium/io/output_iterator.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/object_pointer_collection.hpp>
#include <osmium/visitor.hpp>
#include <osmium/osm/object_comparisons.hpp>
#include <hstore_parser.hpp>
#include <array_parser.hpp>
#include "osmvectortileimpl.hpp"
#include "item_type_conversion.hpp"

void OSMVectorTileImpl::clear(BoundingBox& bbox) {
    m_buffer.clear();
    m_location_handler.clear();
    m_ways_got.clear();
    m_relations_got.clear();
    m_missing_nodes.clear();
    m_missing_ways.clear();
    m_missing_relations.clear();
    m_nodes_table.set_bbox(bbox);
    m_untagged_nodes_table.set_bbox(bbox);
    m_ways_table.set_bbox(bbox);
    m_relations_table.set_bbox(bbox);
}

void OSMVectorTileImpl::add_tags(osmium::builder::Builder* builder, std::string& hstore_content) {
    osmium::builder::TagListBuilder tl_builder(m_buffer, builder);
    pg_array_hstore_parser::HStoreParser hstore (hstore_content);
    while (hstore.has_next()) {
        pg_array_hstore_parser::StringPair kv_pair = hstore.get_next();
        tl_builder.add_tag(kv_pair.first, kv_pair.second);
    }
}

void OSMVectorTileImpl::add_node_refs(osmium::builder::WayBuilder* way_builder, std::string& nodes_array) {
    osmium::builder::WayNodeListBuilder wnl_builder{m_buffer, way_builder};
    pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::Int64Conversion> array_parser(nodes_array);
    while (array_parser.has_next()) {
        // If Int64Conversion::output_type instead of "int64_t", we can easily change the output type of Int64Conversion without
        // having to change it here, too.
        pg_array_hstore_parser::Int64Conversion::output_type node_ref = array_parser.get_next();
        wnl_builder.add_node_ref(osmium::NodeRef(node_ref, osmium::Location()));
        check_node_availability(node_ref);
    }
}

void OSMVectorTileImpl::check_node_availability(const osmium::object_id_type id) {
    try {
        osmium::Location location = m_location_handler.get_node_location(id);
    } catch (osmium::not_found& e) {
        // This exception is thrown if the node could not be found in the location handler.
        m_missing_nodes.insert(id);
    }
}

void OSMVectorTileImpl::add_relation_members(osmium::builder::RelationBuilder* relation_builder,
        std::string& member_types, std::string& member_ids, std::string& member_roles,
        std::set<osmium::object_id_type>* missing_nodes,
        std::set<osmium::object_id_type>* missing_relations,
        std::set<osmium::object_id_type>* relations_got) {
    osmium::builder::RelationMemberListBuilder rml_builder(m_buffer, relation_builder);
    pg_array_hstore_parser::ArrayParser<ItemTypeConversion> array_parser_types(member_types);
    pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::Int64Conversion> array_parser_ids(member_ids);
    pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> array_parser_roles(member_roles);
    while (array_parser_types.has_next() && array_parser_ids.has_next() && array_parser_roles.has_next()) {
        ItemTypeConversion::output_type type = array_parser_types.get_next();
        pg_array_hstore_parser::Int64Conversion::output_type id = array_parser_ids.get_next();
        pg_array_hstore_parser::StringConversion::output_type role = array_parser_roles.get_next();
        rml_builder.add_member(type, id, role.c_str());
        if (type == osmium::item_type::node && missing_nodes) {
            check_node_availability(id);
        } else if (type == osmium::item_type::way && m_config.m_recurse_ways) {
            std::set<osmium::object_id_type>::iterator ways_it = m_ways_got.find(id);
            if (ways_it == m_ways_got.end()) {
                m_missing_ways.insert(id);
            }
        } else if (type == osmium::item_type::relation && missing_relations && relations_got) {
            std::set<osmium::object_id_type>::iterator relations_it = relations_got->find(id);
            if (relations_it == relations_got->end()) {
                missing_relations->insert(id);
            }
        }
    }
}

void OSMVectorTileImpl::get_nodes_inside() {
    PGresult* result = m_nodes_table.run_prepared_bbox_statement("get_nodes_with_tags");
    parse_node_query_result(result, true, 0);
    PQclear(result);
    if (m_config.m_orphaned_nodes) { // If requested by the user, query untagged nodes table, too.
        PGresult* result = m_untagged_nodes_table.run_prepared_bbox_statement("get_nodes_without_tags");
        parse_node_query_result(result, false, 0);
        PQclear(result);
    }
}

void OSMVectorTileImpl::parse_node_query_result(PGresult* result, bool with_tags, osmium::object_id_type id) {
    int field_offset = 0; // will be added to all field IDs if we query a tags column
    if (with_tags && id == 0) {
        field_offset = 2;
    } else if ((with_tags && id != 0) || (!with_tags && id == 0)) {
        field_offset = 1;
    }
    int tuple_count = PQntuples(result);
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        osmium::Location location(atof(PQgetvalue(result, i, 4 + field_offset)), atof(PQgetvalue(result, i, 5 + field_offset)));

        osmium::builder::NodeBuilder builder(m_buffer);
        osmium::Node& node = static_cast<osmium::Node&>(builder.object());
        if (id == 0 && with_tags) {
            node.set_id(strtoll(PQgetvalue(result, i, 1), nullptr, 10));
        } else if (id == 0 && !with_tags) {
            node.set_id(strtoll(PQgetvalue(result, i, 0), nullptr, 10));
        } else {
            node.set_id(id);
        }
        node.set_version(PQgetvalue(result, i, 1 + field_offset));
        node.set_changeset(PQgetvalue(result, i, 3 + field_offset));
        node.set_uid(PQgetvalue(result, i, 0 + field_offset));
        // otherwise the resulting OSM file does not contain the visible=true attribute and some programs behave strange
        node.set_visible(true);
        node.set_timestamp(PQgetvalue(result, i,2 + field_offset));
        builder.set_user("");
        node.set_location(location);
        // Location handler must be called before add_tags(). That's a limitation by Osmium.
        m_location_handler.node(node); // add to location handler to store the location
        if (with_tags) {
            std::string tags_hstore = PQgetvalue(result, i, 0);
            add_tags(&builder, tags_hstore);
        }
    }
    m_buffer.commit();
}

void OSMVectorTileImpl::get_missing_nodes() {
    char* param_values[1];
    char param[25];
    param_values[0] = param;
    for (osmium::object_id_type id : m_missing_nodes) {
        sprintf(param, "%ld", id);
        PGresult* result = m_untagged_nodes_table.run_prepared_statement("get_single_node_without_tags", 1, param_values);
        if (PQntuples(result) > 0) {
            parse_node_query_result(result, false, id);
            PQclear(result);
        } else {
            PGresult* result2 = m_nodes_table.run_prepared_statement("get_single_node_with_tags", 1, param_values);
            parse_node_query_result(result, true, id);
            PQclear(result2);
        }
    }
    m_buffer.commit();
}

void OSMVectorTileImpl::parse_way_query_result(PGresult* result, osmium::object_id_type id) {
    int tuple_count = PQntuples(result);
    int field_offset = 0; // will be added to all field IDs if we query a tags column
    if (id == 0) {
        field_offset = 1;
    }
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        std::string tags_hstore = PQgetvalue(result, i, 0);
        std::string way_nodes = PQgetvalue(result, i, 5 + field_offset);
        osmium::builder::WayBuilder way_builder(m_buffer);
        osmium::Way& way = static_cast<osmium::Way&>(way_builder.object());
        if (id == 0) {
            osmium::object_id_type way_id = strtoll(PQgetvalue(result, i, 1), nullptr, 10);
            m_ways_got.insert(way_id);
            way.set_id(way_id);
        } else {
            way.set_id(id);
        }
        way.set_changeset(PQgetvalue(result, i, 4 + field_offset));
        way.set_uid(PQgetvalue(result, i, 1 + field_offset));
        way.set_version(PQgetvalue(result, i, 2 + field_offset));
        way.set_visible(true);
        way.set_timestamp(PQgetvalue(result, i, 3+ field_offset));
        way_builder.set_user("");
        add_tags(&way_builder, tags_hstore);
        add_node_refs(&way_builder, way_nodes);
    }
}

void OSMVectorTileImpl::get_ways_inside() {
    PGresult* result = m_ways_table.run_prepared_bbox_statement("get_ways");
    parse_way_query_result(result, 0);
    PQclear(result);
    m_buffer.commit();
}

void OSMVectorTileImpl::get_missing_ways() {
    char* param_values[1];
    char param[25];
    param_values[0] = param;
    for (osmium::object_id_type id : m_missing_ways) {
        sprintf(param, "%ld", id);
        PGresult* result = m_ways_table.run_prepared_statement("get_single_way", 1, param_values);
        parse_way_query_result(result, id);
        PQclear(result);
    }
    m_buffer.commit();
}

void OSMVectorTileImpl::parse_relation_query_result(PGresult* result, osmium::object_id_type id) {
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
        osmium::builder::RelationBuilder relation_builder(m_buffer);
        osmium::Relation& relation = static_cast<osmium::Relation&>(relation_builder.object());
        relation.set_id(strtoll(PQgetvalue(result, i, 1), nullptr, 9 + field_offset));
        relation.set_changeset(PQgetvalue(result, i, 4 + field_offset));
        relation.set_uid(PQgetvalue(result, i, 1 + field_offset));
        relation.set_version(PQgetvalue(result, i, 2 + field_offset));
        relation.set_visible(true);
        relation.set_timestamp(PQgetvalue(result, i, 3 + field_offset));
        relation_builder.set_user("");
        add_tags(&relation_builder, tags_hstore);
        std::set<osmium::object_id_type>* missing_nodes_ptr = NULL;
        std::set<osmium::object_id_type>* missing_relations_ptr = NULL;
        if (m_config.m_recurse_nodes) {
            missing_nodes_ptr = &m_missing_nodes;
        }
        if (m_config.m_recurse_relations) {
            missing_relations_ptr = &m_missing_relations;
        }
        add_relation_members(&relation_builder, member_types, member_ids, member_roles, missing_nodes_ptr,
                missing_relations_ptr, &m_relations_got);
    }
}

void OSMVectorTileImpl::get_relations_inside() {
    PGresult* result = m_relations_table.run_prepared_bbox_statement("get_relations");
    parse_relation_query_result(result, 0);
    PQclear(result);
    m_buffer.commit();
}

void OSMVectorTileImpl::get_missing_relations() {
    char* param_values[1];
    char param[25];
    param_values[0] = param;
    for (auto id : m_missing_relations) {
        sprintf(param, "%ld", id);
        PGresult* result = m_ways_table.run_prepared_statement("get_single_relation", 1, param_values);
        parse_way_query_result(result, id);
        PQclear(result);
    }
    m_buffer.commit();
}

void OSMVectorTileImpl::generate_vectortile(std::string& output_path) {
    get_nodes_inside();
    get_ways_inside();
    get_relations_inside();
    if (m_config.m_recurse_relations) {
        get_missing_relations();
    }
    if (m_config.m_recurse_ways) {
        get_missing_ways();
    }
    get_missing_nodes();
    write_file(output_path);
}

void OSMVectorTileImpl::write_file(std::string& path) {
    osmium::io::Header header;
    header.set("generator", "vectortile-generator");
    header.set("copyright", "OpenStreetMap and contributors");
    header.set("attribution", "http://www.openstreetmap.org/copyright");
    header.set("license", "http://opendatacommons.org/licenses/odbl/1-0/");
    osmium::io::File output_file{path};
    osmium::io::overwrite overwrite = osmium::io::overwrite::no;
    if (m_config.m_force) {
        overwrite = osmium::io::overwrite::allow;
    }
    osmium::io::Writer writer{output_file, header, overwrite};
    // We have to merge the buffers and sort the objects. Therefore first all nodes are written, then all ways and as last step
    // all relations.
    sort_buffer_and_write_it(writer);
    writer.close();
}

void OSMVectorTileImpl::create_prepared_statements() {
    std::string query;
    query = (boost::format("SELECT tags, osm_id, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE ST_INTERSECTS(geom, " \
            "ST_MakeEnvelope($1, $2, $3, $4, 4326))") % m_nodes_table.get_name()).str();
    m_nodes_table.create_prepared_statement("get_nodes_with_tags", query, 4);
    query = (boost::format("SELECT tags, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE osm_id = $1") % m_nodes_table.get_name()).str();
    m_nodes_table.create_prepared_statement("get_single_node_with_tags", query, 1);

    query = (boost::format("SELECT osm_id, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE ST_INTERSECTS(geom, " \
            "ST_MakeEnvelope($1, $2, $3, $4, 4326))") % m_untagged_nodes_table.get_name()).str();
    m_untagged_nodes_table.create_prepared_statement("get_nodes_without_tags", query, 4);
    query = (boost::format("SELECT osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE osm_id = $1") % m_untagged_nodes_table.get_name()).str();
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

void OSMVectorTileImpl::sort_buffer_and_write_it(osmium::io::Writer& writer) {
    auto out = osmium::io::make_output_iterator(writer);
    osmium::ObjectPointerCollection objects;
    osmium::apply(m_buffer, objects);
    objects.sort(osmium::object_order_type_id_reverse_version());
    std::unique_copy(objects.cbegin(), objects.cend(), out, osmium::object_equal_type_id());
}
