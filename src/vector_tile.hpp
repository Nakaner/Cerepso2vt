/*
 * vector_tile.hpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */

#ifndef SRC_VECTOR_TILE_HPP_
#define SRC_VECTOR_TILE_HPP_

#include <set>
#include <osmium/memory/buffer.hpp>
#include <osmium/io/writer.hpp>
#include "mytable.hpp"
#include "vectortile_generator_config.hpp"

class VectorTile {
private:
    VectortileGeneratorConfig& m_config;
    /// reference to `untagged_nodes` table
    MyTable& m_untagged_nodes_table;
    /// reference to `nodes` table
    MyTable& m_nodes_table;
    /// reference to `ways` table
    MyTable& m_ways_table;
    /// reference to `relations` table
    MyTable& m_relations_table;

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

    /// bounding box of the tile
    BoundingBox m_bbox;

    /**
     * get relations intersecting this tile
     */
    void get_intersecting_relations();

    /**
     * get ways intersecting this tile
     */
    void get_intersecting_ways();

    /**
     * get nodes inside this tile
     */
    void get_nodes_inside();

    /**
     * get ways intersecting this tile
     */
    void get_ways_inside();

    /**
     * get relations intersecting this tile
     */
    void get_relations_inside();

    /**
     * get all relations which are reference but have not been satisfied yet
     */
    void get_missing_relations();

    /**
     * get all ways which are reference but have not been satisfied yet
     */
    void get_missing_ways();

    /**
     * get all node which are reference but have not been satisfied yet
     */
    void get_missing_nodes();

    /**
     * write vectortile to file
     *
     * You should call the destructor after calling this method (e.g. let this instance go out of scope if it is no pointer).
     */
    void write_file();

public:
    VectorTile(VectortileGeneratorConfig& config, MyTable& untagged_nodes_table, MyTable& nodes_table, MyTable& ways_table,
            MyTable& relations_table);

    /**
     * build the vector tile by querying the database and save it to the disk
     */
    void generate_vectortile();

    /**
     * \brief convert x index of a tile to the WGS84 longitude coordinate of the upper left corner
     *
     * \param tile_x x index
     * \param map_width number of tiles on this zoom level
     *
     * \returns longitude of upper left corner
     */
    static double tile_x_to_merc(const double tile_x, const int map_width);

    /**
     * \brief convert y index of a tile to the WGS84 latitude coordinate of the upper left corner
     *
     * \param tile_y y index
     * \param map_width number of tiles on this zoom level
     *
     * \returns latitude of upper left corner
     */
    static double tile_y_to_merc(const double tile_y, const int map_width);

    /**
     * \brief sort objects in a buffer and write them to a file
     *
     * Code is taken and modified from [Osmium -- OpenStreetMap data manipulation command line tool](http://osmcode.org/osmium)
     * which is Copyright (C) 2013-2016  Jochen Topf <jochen@topf.org> and available under the terms of
     * GNU General Public License version 3 or newer.
     *
     * \param buffer buffer where the objects are stored at
     * \param writer writer which should write the objects to a file
     */
    static void sort_buffer_and_write_it(osmium::memory::Buffer& buffer, osmium::io::Writer& writer);
};



#endif /* SRC_VECTOR_TILE_HPP_ */
