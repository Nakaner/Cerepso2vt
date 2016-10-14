/*
 * mytable.cpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */

#include "mytable.hpp"
#include "hstore_parser.hpp"

BoundingBox::BoundingBox(osmium::geom::Coordinates& south_west, osmium::geom::Coordinates& north_east) :
        min_lon(south_west.x),
        min_lat(south_west.y),
        max_lon(north_east.x),
        max_lat(north_east.y) {}

void MyTable::add_tags(osmium::memory::Buffer& buffer, osmium::builder::Builder& builder, std::string& hstore_content) {
    osmium::builder::TagListBuilder tl_builder(buffer, &builder);
    HStoreParser hstore (hstore_content);
    while (hstore.has_next()) {
        StringPair kv_pair = hstore.get_next();
        tl_builder.add_tag(kv_pair.first, kv_pair.second);
    }
}

std::vector<osmium::Node&> MyTable::get_nodes_inside(osmium::memory::Buffer& node_buffer, location_handler_type& location_handler,
        BoundingBox& bbox) {
    std::vector<osmium::Node&> nodes;
    assert(m_database_connection);
    char const *paramValues[4];
    char param1 [25];
    char param2 [25];
    char param3 [25];
    char param4 [25];
    sprintf(param1, "%ld", bbox.min_lon);
    sprintf(param2, "%ld", bbox.min_lat);
    sprintf(param3, "%ld", bbox.max_lon);
    sprintf(param4, "%ld", bbox.max_lat);
    paramValues[0] = param1;
    paramValues[1] = param2;
    paramValues[2] = param3;
    paramValues[3] = param4;
    PGresult *result = PQexecPrepared(m_database_connection, "get_nodes", 4, paramValues, nullptr, nullptr, 0);
    if ((PQresultStatus(result) != PGRES_COMMAND_OK) && (PQresultStatus(result) != PGRES_TUPLES_OK)) {
        throw std::runtime_error((boost::format("Failed: %1%\n") % PQresultErrorMessage(result)).str());
        PQclear(result);
        return nodes;
    }
    int tuple_count = PQntuples(result);
    if (tuple_count == 0) { // no nodes in this area
        PQclear(result);
        return nodes;
    }
    for (int i = 0; i < tuple_count; i++) { // for each returned row
        int64_t osm_id = strtoll(PQgetvalue(result, i, 0), nullptr, 10);
        std::string tags_hstore = PQgetvalue(result, i, 1);
        osmium::Location location(atof(PQgetvalue(result, 0, 6)), atof(PQgetvalue(result, 0, 7)));
        osmium::builder::NodeBuilder builder(node_buffer);
        osmium::Node& node = static_cast<osmium::Node&>(builder.object());
        node.set_id(strtoll(PQgetvalue(result, i, 0), nullptr, 10));
        node.set_version(PQgetvalue(result, i, 3));
        node.set_changeset(PQgetvalue(result, i, 5));
        node.set_uid(PQgetvalue(result, i, 2));
        node.set_timestamp(PQgetvalue(result, i, 4));
        builder.add_user("");
        node.set_location(location);
        add_tags(node_buffer, builder, tags_hstore);
        // add to location handler to store the location
        location_handler.node(node);
    }
    PQclear(result);
    node_buffer.commit();
    return nodes;
}

void MyTable::create_prepared_statements() {
    Table::create_prepared_statements();
    std::string query;
    query = (boost::format("SELECT osm_id, tags, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE ST_INTERSECTS(geom, " \
            "ST_MakeEnvelope($1, $2, $3, $4, 4326)") % m_name).str();
    create_prepared_statement("get_nodes", query, 4);
    query = (boost::format("SELECT osm_id, tags, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, way_nodes FROM %1% WHERE ST_INTERSECTS(geom, " \
            "ST_MakeEnvelope($1, $2, $3, $4, 4326)") % m_name).str();
    create_prepared_statement("get_ways", query, 4);
    query = (boost::format("SELECT osm_id, tags, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, member_ids, member_types FROM %1% WHERE ST_INTERSECTS(geom, " \
            "ST_MakeEnvelope($1, $2, $3, $4, 4326)") % m_name).str();
    create_prepared_statement("get_relations", query, 4);
    query = (boost::format("SELECT tags, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, ST_X(geom), ST_Y(geom) FROM %1% WHERE osm_id = $1") % m_name).str();
    create_prepared_statement("get_single_node", query, 1);
    query = (boost::format("SELECT tags, osm_uid, osm_version, osm_lastmodified," \
            " osm_changeset, way_nodes FROM %1% WHERE osm_id = $1") % m_name).str();
    create_prepared_statement("get_single_way", query, 1);
}
