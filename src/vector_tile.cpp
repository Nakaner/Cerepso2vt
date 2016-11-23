/*
 * vector_tile.cpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */
#include <osmium/io/any_output.hpp>
#include <osmium/io/file.hpp>
#include <osmium/io/writer.hpp>
#include <osmium/geom/projection.hpp>
#include "vector_tile.hpp"

const double EARTH_CIRCUMFERENCE = 40075016.68;

VectorTile::VectorTile(int x, int y, int zoom, MyTable& untagged_nodes_table, MyTable& nodes_table, MyTable& ways_table,
        MyTable& relations_table, std::string& destination_path) :
        m_x(x),
        m_y(y),
        m_zoom(zoom),
        m_untagged_nodes_table(untagged_nodes_table),
        m_nodes_table(nodes_table),
        m_ways_table(ways_table),
        m_relations_table(relations_table),
        m_destination_path(destination_path),
        m_nodes_buffer(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes),
        m_ways_buffer(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes),
        m_relations_buffer(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes),
        m_additional_nodes_buffer(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes),
        m_additional_ways_buffer(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes),
        m_additional_relations_buffer(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes),
        m_location_handler(m_index) {
    int map_width = 1 << zoom;
    double tile_x_merc = tile_x_to_merc(x, map_width);
    double tile_y_merc = tile_y_to_merc(y, map_width);

    double tile_width_merc = EARTH_CIRCUMFERENCE / map_width;
    double buffer = 0.2 * tile_width_merc;
    const osmium::geom::CRS from_crs (3857);
    const osmium::geom::CRS to_crs (4326);
    osmium::geom::Coordinates south_west (tile_x_merc - buffer, tile_y_merc - tile_width_merc - buffer);
    osmium::geom::Coordinates north_east (tile_x_merc + tile_width_merc + buffer, tile_y_merc + buffer);
    south_west = osmium::geom::transform(from_crs, to_crs, south_west);
    north_east = osmium::geom::transform(from_crs, to_crs, north_east);
    // constructor has already been called before
    m_bbox.convert_to_degree_and_set_coords(south_west, north_east);
}

void VectorTile::get_nodes_inside() {
    m_nodes_table.get_nodes_inside(m_nodes_buffer, m_location_handler, m_bbox);
    m_untagged_nodes_table.get_nodes_inside(m_nodes_buffer, m_location_handler, m_bbox);
}

void VectorTile::get_ways_inside() {
    m_ways_table.get_ways_inside(m_ways_buffer, m_bbox);
}

void VectorTile::generate_vectortile() {
    get_nodes_inside();
    get_ways_inside();
//    get_relations_inside();
//    get_missing_relations();
//    get_missing_ways();
//    get_misssing_nodes();
    write_file();
}

void VectorTile::write_file() {
    osmium::io::Header header;
    header.set("generator", "vectortile-generator");
    header.set("copyright", "OpenStreetMap and contributors");
    header.set("attribution", "http://www.openstreetmap.org/copyright");
    header.set("license", "http://opendatacommons.org/licenses/odbl/1-0/");
    osmium::io::File output_file{m_destination_path};
    osmium::io::Writer writer{output_file, header};
    writer(std::move(m_nodes_buffer));
    writer(std::move(m_ways_buffer));
    writer.close();
}

/* static */ double VectorTile::tile_x_to_merc(double tile_x, int map_width) {
    return EARTH_CIRCUMFERENCE * ((tile_x/map_width) - 0.5);
}

/* static */ double VectorTile::tile_y_to_merc(double tile_y, int map_width) {
    return EARTH_CIRCUMFERENCE * (0.5 - tile_y/map_width);
}
