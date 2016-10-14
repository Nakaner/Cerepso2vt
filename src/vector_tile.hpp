/*
 * vector_tile.hpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */

#ifndef SRC_VECTOR_TILE_HPP_
#define SRC_VECTOR_TILE_HPP_

#include <set>
#include <osmium/handler/node_locations_for_ways.hpp>
#include "mytable.hpp"

using index_type = osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location>;

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

    BoundingBox m_bbox;

    const int BUFFER_SIZE = 10240;

    location_handler_type m_location_handler;

    osmium::memory::Buffer m_nodes_buffer;
    osmium::memory::Buffer m_ways_buffer;
    osmium::memory::Buffer m_relations_buffer;

    std::set<int64_t> m_missing_nodes(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes);
    std::set<int64_t> m_missing_ways(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes);
    std::set<int64_t> m_missing_relations(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes);

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
    VectorTile(int x, int y, int zoom, Table& m_untagged_nodes_table, Table& m_nodes_table, Table& m_ways_table,
            Table& m_relations_table, std::string& destination_path);

    /**
     * build the vector tile by querying the database and save it to the disk
     */
    void generate_vectortile();
};



#endif /* SRC_VECTOR_TILE_HPP_ */
