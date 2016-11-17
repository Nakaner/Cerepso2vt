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
    void add_tags(osmium::memory::Buffer& buffer, osmium::builder::Builder& builder, std::string& hstore_content);

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
};


#endif /* SRC_MYTABLE_HPP_ */
