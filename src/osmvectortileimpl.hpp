/*
 * osmvectortileimpl.hpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */

#ifndef SRC_OSMVECTORTILEIMPL_HPP_
#define SRC_OSMVECTORTILEIMPL_HPP_

#include <vector>
#include <libpq-fe.h>
#include <osmium/memory/buffer.hpp>
#include <osmium/geom/coordinates.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/io/writer.hpp>
#include <postgres_drivers/table.hpp>
#include "bounding_box.hpp"
#include "osm_data_table.hpp"
#include "vectortile_generator_config.hpp"

using index_type = osmium::index::map::SparseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

/**
 * \brief This class queries the database and builds the OSM entities which it will write to the file.
 *
 * It is one implementation which can be used by VectorTile class. Other implementations can build other output formats.
 */
class OSMVectorTileImpl {
private:
    /// reference to the program configuration
    VectortileGeneratorConfig& m_config;

    /// reference to `untagged_nodes` table
    OSMDataTable& m_untagged_nodes_table;
    /// reference to `nodes` table
    OSMDataTable& m_nodes_table;
    /// reference to `ways` table
    OSMDataTable& m_ways_table;
    /// reference to `relations` table
    OSMDataTable& m_relations_table;

    /// default buffer size of Osmium's buffer
    static const size_t BUFFER_SIZE = 10240;

    /// buffer where all built OSM objects will reside
    osmium::memory::Buffer m_buffer;

    /// necessary for m_location_handler
    index_type m_index;
    /// nodes we have already fetched from the database
    location_handler_type m_location_handler;

    /// ways we have already fetched from the database
    std::set<osmium::object_id_type> m_ways_got;
    /// relations we have already fetched from the database
    std::set<osmium::object_id_type> m_relations_got;

    /// list of nodes not retrieved by a spatial query but which are necessary to build the ways
    std::set<osmium::object_id_type> m_missing_nodes;
    /// list of ways not retrieved by a spatial query but which are necessary to complete some relations
    std::set<osmium::object_id_type> m_missing_ways;
    /// list of relations not retrieved by a spatial query but which are referenced by other relations
    std::set<osmium::object_id_type> m_missing_relations;

    /**
     * create all necessary prepared statements for this table
     *
     * This method chooses the suitable prepared statements which are dependend from the table type (point vs. way vs. …).
     * It overwrites the method of the superclass but calls the method of the superclass.
     *
     * \throws std::runtime_error
     */
    void create_prepared_statements();

    /**
     * add a tags to an OSM object
     */
    void add_tags(osmium::builder::Builder* builder, std::string& hstore_content);

    /**
     * \brief add node references to a way
     *
     * \param way_builder pointer to the WayBuilder which builds the way
     * \param nodes_array reference to the string which contains the string representation of the array we received from
     * the database
     */
    void add_node_refs(osmium::builder::WayBuilder* way_builder, std::string& nodes_array);

    /**
     * \brief add members to a relation
     *
     * \param relation_builder pointer to the RelationBuilder which builds the relation
     * \param member_types reference to the string which contains the string representation of the array from member_types column
     * \param member_ids reference to the string which contains the string representation of the array from member_ids column
     * \param member_roles reference to the string which contains the string representation of the array from member_roles column
     */
    void add_relation_members(osmium::builder::RelationBuilder* relation_builder,
            std::string& member_types, std::string& member_ids, std::string& member_roles);

    /**
     * \brief check if a node is available in the location handler and insert it into the list of missing nodes if it is not
     * available in the location handler
     *
     * \param id ID of the node
     */
    void check_node_availability(const osmium::object_id_type id);

    /**
     * \brief Write vectortile to file and insert a job into the jobs database
     *
     * You should call the destructor after calling this method (e.g. let this instance go out of scope if it is no pointer).
     *
     * \param path path where to write the file
     */
    void write_file(std::string& path);

    /**
     * \brief Get all nodes in the tile
     *
     * \throws std::runtime_error
     */
    void get_nodes_inside();

    /**
     * \brief Parse the response of the database after querying nodes
     *
     * Does not clean up memory afterwards!
     *
     * \param result query result
     * \param with_tags True if nodes table was queried
     * \param id OSM object ID to use to construct the objects. Use 0 if it was a spatial query.
     */
    void parse_node_query_result(PGresult* result, bool with_tags, osmium::object_id_type id);

    /**
     * \brief Parse the response of the database after querying ways
     *
     * Does not clean up memory afterwards!
     *
     * \param result query result
     * \param id OSM object ID to use to construct the objects. Use 0 if it was a spatial query.
     */
    void parse_way_query_result(PGresult* result, osmium::object_id_type id);

    /**
     * \brief Parse the response of the database after querying relations
     *
     * Does not clean up memory afterwards!
     *
     * \param result query result
     * \param id OSM object ID to use to construct the objects. Use 0 if it was a spatial query.
     */
    void parse_relation_query_result(PGresult* result, osmium::object_id_type id);

    /**
     * \brief Get all missing nodes
     */
    void get_missing_nodes();

    /**
     * \brief Get all ways inside the tile
     */
    void get_ways_inside();
    /**
     * \brief Get all missing ways
     */
    void get_missing_ways();

    /**
     * \brief Get all relations inside the tile
     */
    void get_relations_inside();
    /**
     * \brief Get all missing relations
     */
    void get_missing_relations();

    /**
     * \brief sort objects in a buffer and write them to a file
     *
     * Code is taken and modified from [Osmium -- OpenStreetMap data manipulation command line tool](http://osmcode.org/osmium)
     * which is Copyright (C) 2013-2016  Jochen Topf <jochen@topf.org> and available under the terms of
     * GNU General Public License version 3 or newer.
     *
     * \param writer writer which should write the objects to a file
     */
    void sort_buffer_and_write_it(osmium::io::Writer& writer);

public:
    OSMVectorTileImpl(VectortileGeneratorConfig& config, OSMDataTable& untagged_nodes_table, OSMDataTable& nodes_table, OSMDataTable& ways_table,
            OSMDataTable& relations_table) :
            m_config(config),
            m_untagged_nodes_table(untagged_nodes_table),
            m_nodes_table(nodes_table),
            m_ways_table(ways_table),
            m_relations_table(relations_table),
            m_buffer(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes),
            m_location_handler(m_index) {
        create_prepared_statements();
    };

    /**
     * \brief Reset the Osmium buffer and all sets (using the clear() method).
     * Change the bounding box of all table members of this class.
     *
     * \param bbox new bounding box
     */
    void clear(BoundingBox& bbox);

    /**
     * \brief build the vector tile
     *
     * This method will be called by the class VectorTile to start the work.
     *
     * \param output_path location where to write the tile
     */
    void generate_vectortile(std::string& output_path);
};


#endif /* SRC_OSMVECTORTILEIMPL_HPP_ */
