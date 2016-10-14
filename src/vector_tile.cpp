/*
 * vector_tile.cpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */

const double EARTH_CIRCUMFERENCE = 40075016.68;

VectorTile::VectorTile(int x, int y, int zoom, Table& untagged_nodes_table, Table& nodes_table, Table& ways_table,
        Table& relations_table, std::string& destination_path) :
        m_x(x),
        m_y(y),
        m_zoom(zoom),
        m_untagged_nodes_table(untagged_nodes_table),
        m_nodes_table(nodes_table),
        m_ways_table(ways_table),
        m_relations_table(relations_table),
        m_destination_path(destination_path) {
    int map_width = 1 << zoom;
    double tile_x_merc = tile_x_to_merc(x, map_width);
    double tile_y_merc = tile_y_to_merc(y, map_width);

    double tile_width_merc = EARTH_CIRCUMFERENCE / map_width;
    double buffer = 0.2 * tile_width_merc;
    osmium::geom::Coordinates south_west (tile_x_merc - buffer, tile_y_merc - tile_width_merc - buffer);
    osmium::geom::Coordinates north_east (tile_x_merc + tile_width_merc + buffer, tile_y_merc + buffer);
    const osmium::geom::CRS from_crs (3857);
    const osmium::geom::CRS to_crs (4326);
    osmium::geom::transform(from_crs, to_crs, south_west);
    osmium::geom::transform(from_crs, to_crs, north_east);
    m_bbox = BoundingBox(south_west, north_east);
    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();
    auto location_index = map_factory.create_map("sparse_mmap_array");
    location_handler_type m_location_handler(*location_index);
}

void VectorTile::get_nodes_inside() {
    m_nodes_table.get_nodes_inside(m_nodes_buffer, m_location_handler, m_bbox);
    m_untagged_nodes_table.get_nodes_inside(m_nodes_buffer, m_location_handler, m_bbox);
}

void VectorTile::generate_vectortile() {
    get_nodes_inside();
    get_ways_inside();
    get_relations_inside();
    get_missing_relations();
    get_missing_ways();
    get_misssing_nodes();
    write_file();
}

double tile_x_to_merc(double tile_x, int map_width) {
    return EARTH_CIRCUMFERENCE * ((tile_x/map_width) - 0.5);
}

double tile_y_to_merc(double tile_y, int map_width) {
    return EARTH_CIRCUMFERENCE * (0.5 - tile_y/map_width);
}
