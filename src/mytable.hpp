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
#include <postgres-drivers/include/table.hpp>

/**
 * Bounding box which represents a tile in EPSG:4326 including a buffer.
 */
struct BoundingBox {
    double min_lon;
    double min_lat;
    double max_lon;
    double max_lat;

    BoundingBox(osmium::geom::Coordinates& south_west, osmium::geom::Coordinates& north_east);
};

using TagVector = std::vector<std::string, std::string>;

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
     */
    void create_prepared_statements();

    /**
     * add a tags to an OSM object
     */
    void add_tags(osmium::memory::Buffer& buffer, osmium::builder::Builder& builder, std::string& hstore_content);

public:
    /**
     * get all nodes
     */
    std::vector<osmium::Node&> get_nodes_inside(osmium::memory::Buffer& node_buffer, location_handler_type& location_handler,
            BoundingBox& bbox);
};


#endif /* SRC_MYTABLE_HPP_ */
