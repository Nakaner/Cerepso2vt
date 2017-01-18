/*
 * bounding_box.cpp
 *
 *  Created on: 17.11.2016
 *      Author: michael
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <osmium/geom/util.hpp>
#include "bounding_box.hpp"

/// \todo check if this is the "right" circumfence
const double EARTH_CIRCUMFERENCE = 40075016.68;

BoundingBox::BoundingBox(VectortileGeneratorConfig& config) :
    BoundingBox(config.m_x, config.m_y, config.m_zoom) {}

BoundingBox::BoundingBox(int x, int y, int zoom) :
        m_x(x),
        m_y(y),
        m_zoom(zoom) {
    int map_width = 1 << zoom;
    double tile_x_merc = tile_x_to_merc(x, map_width);
    double tile_y_merc = tile_y_to_merc(y, map_width);
    double tile_width_merc = EARTH_CIRCUMFERENCE / map_width;
    double buffer = 0.2 * tile_width_merc;
    const osmium::geom::CRS from_crs (3857);
    const osmium::geom::CRS to_crs (4326);
    // check that the tile does not cross the antimeridian
    double west = tile_x_merc - buffer;
    if (west < -EARTH_CIRCUMFERENCE / 2) {
        west = -EARTH_CIRCUMFERENCE / 2;
    }
    double north = tile_y_merc = tile_y_merc + buffer;
    if (north > EARTH_CIRCUMFERENCE / 2) {
        north = EARTH_CIRCUMFERENCE / 2;
    }
    double east = tile_x_merc + tile_width_merc + buffer;
    if (east > EARTH_CIRCUMFERENCE / 2) {
        east = EARTH_CIRCUMFERENCE / 2;
    }
    double south = tile_y_merc = tile_y_merc - tile_width_merc - buffer;
    if (south < -EARTH_CIRCUMFERENCE / 2) {
        south = -EARTH_CIRCUMFERENCE / 2;
    }
    // transform to EPSG:4326
    osmium::geom::Coordinates south_west (west, south);
    osmium::geom::Coordinates north_east (east, north);
    south_west = osmium::geom::transform(from_crs, to_crs, south_west);
    north_east = osmium::geom::transform(from_crs, to_crs, north_east);
    // convert from coordinates radians to degree
    convert_to_degree_and_set_coords(south_west, north_east);
}

bool BoundingBox::operator!=(BoundingBox& other){
    if (this->m_max_lat != other.m_max_lat || this->m_min_lat != other.m_min_lat || this->m_max_lon != other.m_max_lon
            || this->m_min_lon != other.m_min_lon) {
        return false;
    }
    return true;
}

void BoundingBox::convert_to_degree_and_set_coords(osmium::geom::Coordinates& south_west, osmium::geom::Coordinates& north_east) {
    m_min_lon = radians_to_degree(south_west.x);
    m_min_lat = radians_to_degree(south_west.y);
    m_max_lon = radians_to_degree(north_east.x);
    m_max_lat = radians_to_degree(north_east.y);
}

bool BoundingBox::is_valid() {
    if ((m_min_lon == m_max_lon) || (m_min_lat == m_max_lat)) {
        return false;
    }
    return true;
}

/* static */ double BoundingBox::tile_x_to_merc(const double tile_x, const int map_width) {
    return EARTH_CIRCUMFERENCE * ((tile_x/map_width) - 0.5);
}

/* static */ double BoundingBox::tile_y_to_merc(const double tile_y, const int map_width) {
    return EARTH_CIRCUMFERENCE * (0.5 - tile_y/map_width);
}

/* static */ int BoundingBox::zoom_to_map_with(const int zoom) {
    return 1 << zoom;
}

/* static */ double BoundingBox::radians_to_degree(double coordinate) {
    return (coordinate / osmium::geom::PI) * 180;
}

/*static*/ std::vector<BoundingBox> BoundingBox::read_tiles_list(const char* filename) {
    std::vector<BoundingBox> tiles;
    FILE * fp;
    fp = fopen(filename, "r");
    unsigned int x;
    unsigned int y;
    unsigned int zoom;
    while (fscanf(fp, "%u/%u/%u", &zoom, &x, &y) == 3) {
        tiles.push_back(BoundingBox(x, y, zoom));
    }
    return tiles;
}
