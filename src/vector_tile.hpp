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
#include "mytable.hpp"

class VectorTile {
private:
    int m_x;
    int m_y;
    int m_zoom;
    MyTable& m_untagged_nodes_table;
    MyTable& m_nodes_table;
    MyTable& m_ways_table;
    MyTable& m_relations_table;
    std::string& m_destination_path;

    static const size_t BUFFER_SIZE = 10240;

    osmium::memory::Buffer m_nodes_buffer;
    osmium::memory::Buffer m_ways_buffer;
    osmium::memory::Buffer m_relations_buffer;

    osmium::memory::Buffer m_additional_nodes_buffer;
    osmium::memory::Buffer m_additional_ways_buffer;
    osmium::memory::Buffer m_additional_relations_buffer;

    index_type m_index; // necessary for m_location_handler
    location_handler_type m_location_handler;

    std::set<int64_t> m_missing_nodes;
    std::set<int64_t> m_missing_ways;
    std::set<int64_t> m_missing_relations;

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
    VectorTile(int x, int y, int zoom, MyTable& m_untagged_nodes_table, MyTable& m_nodes_table, MyTable& m_ways_table,
            MyTable& m_relations_table, std::string& destination_path);

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
    static double tile_x_to_merc(double tile_x, int map_width);

    /**
     * \brief convert y index of a tile to the WGS84 latitude coordinate of the upper left corner
     *
     * \param tile_y y index
     * \param map_width number of tiles on this zoom level
     *
     * \returns latitude of upper left corner
     */
    static double tile_y_to_merc(double tile_y, int map_width);
};



#endif /* SRC_VECTOR_TILE_HPP_ */
