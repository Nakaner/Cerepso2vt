/*
 * mytable.hpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */

#ifndef SRC_MYTABLE_HPP_
#define SRC_MYTABLE_HPP_

#include <vector>
#include <libpq-fe.h>
#include <osmium/memory/buffer.hpp>
#include <osmium/geom/coordinates.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <table.hpp>
#include "bounding_box.hpp"

using TagVector = std::vector<std::string, std::string>;

using index_type = osmium::index::map::SparseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

/**
 * This class extends Table by some methods for querying objects from the database and creating OSM objects out of them.
 */
class MyTable : public postgres_drivers::Table {
private:
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
    void add_tags(osmium::memory::Buffer& buffer, osmium::builder::Builder* builder, std::string& hstore_content);

    /**
     * \brief add node references to a way
     *
     * \param ways_buffer buffer where the way resides
     * \param way_builder pointer to the WayBuilder which builds the way
     * \param nodes_array reference to the string which contains the string representation of the array we received from
     * \param location_handler reference to a location handler which is used to look up if a node referenced by a way was retrieved by
     *        the spatial node queries
     * \param missing_nodes set where to add all nodes which are not available
     * the database
     */
    void add_node_refs(osmium::memory::Buffer& ways_buffer, osmium::builder::WayBuilder* way_builder, std::string& nodes_array,
            location_handler_type& location_handler, std::set<osmium::object_id_type>& missing_nodes);

    /**
     * \brief add members to a relation
     *
     * \param relation_buffer buffer where the relation resides
     * \param relation_builder pointer to the RelationBuilder which builds the relation
     * \param member_types reference to the string which contains the string representation of the array from member_types column
     * \param member_ids reference to the string which contains the string representation of the array from member_ids column
     * \param member_roles reference to the string which contains the string representation of the array from member_roles column
     * \param location_handler reference to the location handler
     * \param missing_nodes reference to a set where node IDs are stored which have not been retrieved from the database yet
     * \param missing_ways dto. for ways
     * \param missing_relations dto. for relations
     * \param ways_got reference to a set where way IDs are stored which have been retrieved from the database yet
     * \param relations_got dto. for relations
     *
     * The parameters missing_relations and relations_got are pointers because there are use cases where you do not want that
     * items are added to these sets. If you don't want items to be added to them, give a nullptr to this method.
     */
    void add_relation_members(osmium::memory::Buffer& relation_buffer, osmium::builder::RelationBuilder* relation_builder,
            std::string& member_types, std::string& member_ids, std::string& member_roles, location_handler_type& location_handler,
            std::set<osmium::object_id_type>& missing_nodes, std::set<osmium::object_id_type>& missing_ways,
            std::set<osmium::object_id_type>* missing_relations, std::set<osmium::object_id_type>& ways_got,
            std::set<osmium::object_id_type>* relations_got);

    /**
     * \brief helper method to build an array of bbox parameters
     *
     * \param bbox bounding box
     * \param params_array (size == 4) array to be filled with the parameters derived from bbox argument
     */
    void build_bbox_query_params(BoundingBox& bbox, char** const params_array);

    /**
     * \brief check if a node is available in the location handler and insert it into the list of missing nodes if it is not
     * available in the location handler
     *
     * \param location_handler location handler
     * \param missing_nodes list of missing nodes
     * \param id ID of the node
     */
    void check_node_availability(location_handler_type& location_handler, std::set<osmium::object_id_type>& missing_nodes,
            const osmium::object_id_type id);

public:
    MyTable(const char* table_name, postgres_drivers::Config& config, postgres_drivers::Columns& columns) :
            postgres_drivers::Table(table_name, config, columns) {
        create_prepared_statements();
    };

    /**
     * \brief Get all nodes in the tile
     *
     * \param node_buffer buffer where to write the nodes
     * \param location_handler location handler which handles the location of the nodes
     * \param bbox bounding box specifying the extend of the tile (including a buffer around its edges)
     *
     * \throws std::runtime_error
     */
    void get_nodes_inside(osmium::memory::Buffer& node_buffer, location_handler_type& location_handler,
            BoundingBox& bbox);

    /**
     * \brief Get all missing nodes
     *
     * \param node_buffer buffer where to write the nodes
     * \param missing_nodes nodes to be fetched from the database
     *
     * \throws std::runtime_error
     */
    void get_missing_nodes(osmium::memory::Buffer& node_buffer, std::set<osmium::object_id_type>& missing_nodes);

    /**
     * \brief Get all ways inside the tile
     *
     * \param ways_buffer buffer where to write the ways
     * \param bbox bounding box specifying the extend of the tile (including a buffer around its edges)
     * \param location_handler reference to a location handler which is used to look up if a node referenced by a way was retrieved by
     *        the spatial node queries
     * \param missing_nodes set where to add all nodes which are not available
     * \param ways_got set where the IDs of all ways returned by the database queries will be stored
     *
     * \throws std::runtime_error
     */
    void get_ways_inside(osmium::memory::Buffer& ways_buffer, BoundingBox& bbox, location_handler_type& location_handler,
            std::set<osmium::object_id_type>& missing_nodes, std::set<osmium::object_id_type>& ways_got);
    /**
     * \brief Get all missing ways
     *
     * \param node_buffer buffer where to write the nodes
     * \param missing_ways ways to be fetched from the database
     * \param location_handler location handler managing node locations
     * \param missing_nodes list of nodes to be fetched from the database. This method will add all nodes which
     * are referenced by missing ways and have not been retrieved yet from the database.
     *
     * \throws std::runtime_error
     */
    void get_missing_ways(osmium::memory::Buffer& way_buffer, std::set<osmium::object_id_type>& missing_ways,
            location_handler_type& location_handler, std::set<osmium::object_id_type>& missing_nodes);

    /**
     * \brief Get all ways inside the tile
     *
     * \param ways_buffer buffer where to write the ways
     * \param bbox bounding box specifying the extend of the tile (including a buffer around its edges)
     *
     * \throws std::runtime_error
     */
    void get_relations_inside(osmium::memory::Buffer& relations_buffer, BoundingBox& bbox, location_handler_type& location_handler,
            std::set<osmium::object_id_type>& missing_nodes, std::set<osmium::object_id_type>& missing_ways,
            std::set<osmium::object_id_type>& missing_relations, std::set<osmium::object_id_type>& ways_got,
            std::set<osmium::object_id_type>& relations_got);
    /**
     * \brief Get all missing relations
     *
     * \param relation_buffer buffer where to write the nodes
     * \param missing_relations relations to be fetched from the database
     * \param missing_ways ways to be fetched from the database
     * \param ways_got ways which have already been fetched from the database
     * \param location_handler location handler managing node locations
     * \param missing_nodes list of nodes to be fetched from the database. This method will add all nodes which
     * are referenced by missing ways and have not been retrieved yet from the database.
     *
     * \throws std::runtime_error
     */
    void get_missing_relations(osmium::memory::Buffer& relation_buffer, std::set<osmium::object_id_type>& missing_relations,
            std::set<osmium::object_id_type>& missing_ways, std::set<osmium::object_id_type>& ways_got,
            location_handler_type& location_handler, std::set<osmium::object_id_type>& missing_nodes);
};


#endif /* SRC_MYTABLE_HPP_ */
