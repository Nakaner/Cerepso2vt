/*
 * bounding_box.cpp
 *
 *  Created on: 17.11.2016
 *      Author: michael
 */

#include "bounding_box.hpp"
#include <osmium/geom/util.hpp>

BoundingBox::BoundingBox() {}

BoundingBox::BoundingBox(osmium::geom::Coordinates& south_west, osmium::geom::Coordinates& north_east) :
        min_lon(radians_to_degree(south_west.x)),
        min_lat(radians_to_degree(south_west.y)),
        max_lon(radians_to_degree(north_east.x)),
        max_lat(radians_to_degree(north_east.y)) {}

void BoundingBox::convert_to_degree_and_set_coords(osmium::geom::Coordinates& south_west, osmium::geom::Coordinates& north_east) {
    min_lon = radians_to_degree(south_west.x);
    min_lat = radians_to_degree(south_west.y);
    max_lon = radians_to_degree(north_east.x);
    max_lat = radians_to_degree(north_east.y);
}

bool BoundingBox::is_valid() {
    if ((min_lon == max_lon) || (min_lat == max_lat)) {
        return false;
    }
    return true;
}

/* static */ double BoundingBox::radians_to_degree(double coordinate) {
    return (coordinate / osmium::geom::PI) * 180;
}
