/*
 * bounding_box.hpp
 *
 *  Created on: 17.11.2016
 *      Author: michael
 */

#ifndef SRC_BOUNDING_BOX_HPP_
#define SRC_BOUNDING_BOX_HPP_

#include <vector>
#include <osmium/geom/coordinates.hpp>
#include <osmium/geom/projection.hpp>
#include "vectortile_generator_config.hpp"

/**
 * \brief Bounding box which represents a tile in EPSG:4326 including a buffer.
 */
class BoundingBox {
    /**
     * \brief Convert coordinates from radians to degree and set them.
     *
     * Similar behavior to BoundingBox::BoundingBox(osmium::geom::Coordinates& south_west, osmium::geom::Coordinates& north_east).
     *
     * \param south_west coordinate pair of south-west corner of the tile in radians
     * \param north_east coordinate pair of south-west corner of the tile in radians
     */
    void convert_to_degree_and_set_coords(osmium::geom::Coordinates& south_west, osmium::geom::Coordinates& north_east);

public:
    // This class is that simple that we do not hide any of its members.
    /// x index of the tile this bounding box represents
    int m_x;
    /// y index of the tile this bounding box represents
    int m_y;
    /// zoom level of the tile this bounding box represents
    int m_zoom;
    /// longitude at south-west corner
    double m_min_lon = 0;
    /// latitude at south-west corner
    double m_min_lat = 0;
    /// longitude at north-east corner
    double m_max_lon = 0;
    /// latitude at north-east corner
    double m_max_lat = 0;

    BoundingBox() = delete;

    /**
     * \brief constructor which converts from tile IDs to the internal representation of coordinates in this class
     *
     * \param x x index of the tile
     * \param y y index of the tile
     * \param zoom zoom level of the tile
     */
    BoundingBox(int x, int y, int zoom);

    /**
     * \brief Constructor which converts from tile ID to the internal representation of coordinates in this class.
     * The tile ID is read from the program configuration (single tile mode).
     *
     * \param config reference to VectortileGeneratorConfig
     */
    BoundingBox(VectortileGeneratorConfig& config);

    /**
     * \brief Compare two bounding boxes.
     *
     * This method was written for some unit tests to make them look nice.
     */
    bool operator!=(BoundingBox& other);

    /**
     * \brief check if the bounding box is not collapsed
     *
     * \returns true if the bounding box is ok
     */
    bool is_valid();

    /**
     * \brief calculate the number of tiles in x/y direction at a given zoom level
     *
     * The implementation uses bitshifts.
     *
     * \param zoom zoom level
     *
     * \returns map width
     */
    static int zoom_to_map_with(const int zoom);

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
     * \brief convert a coordinated from radians to degree
     *
     * \param coordinate the coordinate in radians
     * \returns coordinate in degree
     */
    static double radians_to_degree(double coordinate);

    /**
     * \brief Read list of expired tiles from file.
     *
     * \param filename null-terminated string containing the file name to read from
     *
     * \returns vector with one bounding box per tile
     */
    static std::vector<BoundingBox> read_tiles_list(const char* filename);
};

#endif /* SRC_BOUNDING_BOX_HPP_ */
