/*
 * bounding_box.hpp
 *
 *  Created on: 17.11.2016
 *      Author: michael
 */

#ifndef SRC_BOUNDING_BOX_HPP_
#define SRC_BOUNDING_BOX_HPP_

#include <osmium/geom/coordinates.hpp>

/**
 * \brief Bounding box which represents a tile in EPSG:4326 including a buffer.
 */
class BoundingBox {
public:
    // This class is that simple that we do not hide any of its members.
    double min_lon = 0;
    double min_lat = 0;
    double max_lon = 0;
    double max_lat = 0;

    BoundingBox();

    /*
     * \brief constructor which converts from radians to degree
     *
     * \param south_west coordinate pair of south-west corner of the tile in radians
     * \param north_east coordinate pair of south-west corner of the tile in radians
     */
    BoundingBox(osmium::geom::Coordinates& south_west, osmium::geom::Coordinates& north_east);

    /**
     * \brief Convert coordinates from radians to degree and set them.
     *
     * Similar behavior to BoundingBox::BoundingBox(osmium::geom::Coordinates& south_west, osmium::geom::Coordinates& north_east).
     *
     * \param south_west coordinate pair of south-west corner of the tile in radians
     * \param north_east coordinate pair of south-west corner of the tile in radians
     */
    void convert_to_degree_and_set_coords(osmium::geom::Coordinates& south_west, osmium::geom::Coordinates& north_east);

    /**
     * \brief check if the bounding box is not collapsed
     *
     * \returns true if the bounding box is ok
     */
    bool is_valid();

    /**
     * \brief convert a coordinated from radians to degree
     *
     * \param coordinate the coordinate in radians
     * \returns coordinate in degree
     */
    static double radians_to_degree(double coordinate);
};

#endif /* SRC_BOUNDING_BOX_HPP_ */
