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
     * the database
     */
    void add_node_refs(osmium::memory::Buffer& ways_buffer, osmium::builder::WayBuilder* way_builder, std::string& nodes_array);

    /**
     * \brief helper method to build an array of bbox parameters
     *
     * \param bbox bounding box
     * \param params_array (size == 4) array to be filled with the parameters derived from bbox argument
     */
    void build_bbox_query_params(BoundingBox& bbox, char** const params_array);

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
     * \brief Get all ways inside the tile
     *
     * \param ways_buffer buffer where to write the ways
     * \param bbox bounding box specifying the extend of the tile (including a buffer around its edges)
     *
     * \throws std::runtime_error
     */
    void get_ways_inside(osmium::memory::Buffer& ways_buffer, BoundingBox& bbox);
};


#endif /* SRC_MYTABLE_HPP_ */
