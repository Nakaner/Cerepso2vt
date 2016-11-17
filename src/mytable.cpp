/*
 * mytable.cpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */

#include "mytable.hpp"
#include <osmium/builder/osm_object_builder.hpp>
#include "hstore_parser.hpp"

void MyTable::add_tags(osmium::memory::Buffer& buffer, osmium::builder::Builder& builder, std::string& hstore_content) {
//    osmium::builder::TagListBuilder tl_builder(buffer, &builder);
//    HStoreParser hstore (hstore_content);
//    while (hstore.has_next()) {
//        StringPair kv_pair = hstore.get_next();
//        tl_builder.add_tag(kv_pair.first, kv_pair.second);
//    }
}

void MyTable::get_nodes_inside(osmium::memory::Buffer& node_buffer, location_handler_type& location_handler,
        BoundingBox& bbox) {
    assert(m_database_connection);
    assert(m_columns.get_type() == postgres_drivers::TableType::POINT || m_columns.get_type() == postgres_drivers::TableType::UNTAGGED_POINT);
    // build array of parameters for prepared statement
    char const *paramValues[4];
    char param1 [25];
    char param2 [25];
    char param3 [25];
    char param4 [25];
    sprintf(param1, "%f", bbox.min_lon);
    sprintf(param2, "%f", bbox.min_lat);
    sprintf(param3, "%f", bbox.max_lon);
    sprintf(param4, "%f", bbox.max_lat);
    paramValues[0] = param1;
    paramValues[1] = param2;
    paramValues[2] = param3;
    paramValues[3] = param4;

    PGresult *result;
    if  (m_columns.get_type() == postgres_drivers::TableType::POINT) {
        result = PQexecPrepared(m_database_connection, "get_nodes_with_tags", 4, paramValues, nullptr, nullptr, 0);
    } else { // other types than UNTAGGED_POINT are not possible because we have an assertion checking that a few lines above
        result = PQexecPrepared(m_database_connection, "get_nodes_without_tags", 4, paramValues, nullptr, nullptr, 0);
    }
    if ((PQresultStatus(result) != PGRES_COMMAND_OK) && (PQresultStatus(result) != PGRES_TUPLES_OK)) {
        throw std::runtime_error((boost::format("Failed: %1%\n") % PQresultErrorMessage(result)).str());
        PQclear(result);
        return;
    }
    int tuple_count = PQntuples(result);
    if (tuple_count == 0) { // no nodes in this area
        PQclear(result);
        return;
    }

    int field_offset = 0; // will be added to all field IDs if we query a tags column
    if (m_columns.get_type() == postgres_drivers::TableType::POINT) {
        field_offset = 1;
    }

    for (int i = 0; i < tuple_count; i++) { // for each returned row
//        std::string tags_hstore = PQgetvalue(result, i, 0);
        osmium::Location location(atof(PQgetvalue(result, i, 5 + field_offset)), atof(PQgetvalue(result, i, 6 + field_offset)));

        osmium::builder::NodeBuilder builder(node_buffer);
        osmium::Node& node = static_cast<osmium::Node&>(builder.object());
        node.set_id(strtoll(PQgetvalue(result, i, 0 + field_offset), nullptr, 10));
        node.set_version(PQgetvalue(result, i, 2 + field_offset));
        node.set_changeset(PQgetvalue(result, i, 4 + field_offset));
        node.set_uid(PQgetvalue(result, i, 1 + field_offset));
        // otherwise the resulting OSM file does not contain the visible=true attribute and some programs behave strange
        node.set_visible(true);
        char timestamp[21];
        strncpy(timestamp, PQgetvalue(result, i, 3 + field_offset), 20);
        timestamp[20] = '\0';
        node.set_timestamp(timestamp);
        builder.set_user("");
        node.set_location(location);
//        add_tags(node_buffer, builder, tags_hstore);
        // add to location handler to store the location
        location_handler.node(node);
    }
    PQclear(result);
    node_buffer.commit();
}

void MyTable::create_prepared_statements() {
    Table::create_prepared_statements();
    std::string query;
    switch (m_columns.get_type()) {
    case postgres_drivers::TableType::POINT :
        query = (boost::format("SELECT tags, osm_id, osm_uid, osm_version, osm_lastmodified," \
                " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE ST_INTERSECTS(geom, " \
                "ST_MakeEnvelope($1, $2, $3, $4, 4326))") % m_name).str();
        create_prepared_statement("get_nodes_with_tags", query, 4);
        query = (boost::format("SELECT tags, osm_uid, osm_version, osm_lastmodified," \
                " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE osm_id = $1") % m_name).str();
        create_prepared_statement("get_single_node_with_tags", query, 1);
        break;
    case postgres_drivers::TableType::UNTAGGED_POINT :
        query = (boost::format("SELECT osm_id, osm_uid, osm_version, osm_lastmodified," \
                " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE ST_INTERSECTS(geom, " \
                "ST_MakeEnvelope($1, $2, $3, $4, 4326))") % m_name).str();
        create_prepared_statement("get_nodes_without_tags", query, 4);
        query = (boost::format("SELECT osm_uid, osm_version, osm_lastmodified," \
                " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE osm_id = $1") % m_name).str();
        create_prepared_statement("get_single_node_without_tags", query, 1);
        break;
    case postgres_drivers::TableType::WAYS_LINEAR :
        query = (boost::format("SELECT osm_id, tags, osm_uid, osm_version, osm_lastmodified," \
                " osm_changeset, way_nodes FROM %1% WHERE ST_INTERSECTS(geom, " \
                "ST_MakeEnvelope($1, $2, $3, $4, 4326))") % m_name).str();
        create_prepared_statement("get_ways", query, 4);
        query = (boost::format("SELECT tags, osm_uid, osm_version, osm_lastmodified," \
                " osm_changeset, way_nodes FROM %1% WHERE osm_id = $1") % m_name).str();
        create_prepared_statement("get_single_way", query, 1);
        break;
    case postgres_drivers::TableType::RELATION_OTHER :
        query = (boost::format("SELECT osm_id, tags, osm_uid, osm_version, osm_lastmodified," \
                " osm_changeset, member_ids, member_types FROM %1% WHERE ST_INTERSECTS(geom, " \
                "ST_MakeEnvelope($1, $2, $3, $4, 4326))") % m_name).str();
        create_prepared_statement("get_relations", query, 4);
        break;
    default :
        throw std::runtime_error("unsupported table type");
    }
}
