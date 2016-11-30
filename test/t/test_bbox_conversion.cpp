/*
 * test_bbox_conversion.cpp
 *
 *  Created on:  2016-11-30
 *      Author: Michael Reichert
 */

#include "catch.hpp"
#include "util.hpp"
#include <bounding_box.hpp>

TEST_CASE("Test coordinate conversion of BoundingBox class, zoom level 12") {
    int zoom = 12;

    SECTION("convert x index of tile ID to longitude, north-east of coordinate origin") {
        REQUIRE(static_cast<int>(BoundingBox::tile_x_to_merc(2143, BoundingBox::zoom_to_map_with(zoom))) == 929474);
    }

    SECTION("convert y index of tile ID to latitude, north-east of coordinate origin") {
        REQUIRE(static_cast<int>(BoundingBox::tile_y_to_merc(1405, BoundingBox::zoom_to_map_with(zoom))) == 6291073);
    }

    SECTION("convert x index of tile ID to longitude, south-west of coordinate origin") {
        REQUIRE(static_cast<int>(BoundingBox::tile_x_to_merc(1518, BoundingBox::zoom_to_map_with(zoom))) == -5185487);
    }

    SECTION("convert y index of tile ID to latitude, south-west of coordinate origin") {
        REQUIRE(static_cast<int>(BoundingBox::tile_y_to_merc(2326, BoundingBox::zoom_to_map_with(zoom))) == -2719935);
    }

    SECTION("convert x index of tile ID to longitude, south-east of coordinate origin") {
        REQUIRE(static_cast<int>(BoundingBox::tile_x_to_merc(3938, BoundingBox::zoom_to_map_with(zoom))) == 18491645);
    }

    SECTION("convert y index of tile ID to latitude, south-east of coordinate origin") {
        REQUIRE(static_cast<int>(BoundingBox::tile_y_to_merc(2721, BoundingBox::zoom_to_map_with(zoom))) == -6584591);
    }

    SECTION("convert x index of tile ID to longitude, north-west of coordinate origin") {
        REQUIRE(static_cast<int>(BoundingBox::tile_x_to_merc(648, BoundingBox::zoom_to_map_with(zoom))) == -13697515);
    }

    SECTION("convert y index of tile ID to latitude, north-west of coordinate origin") {
        REQUIRE(static_cast<int>(BoundingBox::tile_y_to_merc(1401, BoundingBox::zoom_to_map_with(zoom))) == 6330208);
    }

    SECTION("convert x index of tile ID to longitude, at Greenwich meridian") {
        REQUIRE(static_cast<int>(BoundingBox::tile_x_to_merc(2047, BoundingBox::zoom_to_map_with(zoom))) == -9783);
    }

    SECTION("convert y index of tile ID to latitude, at Greenwich meridian") {
        REQUIRE(static_cast<int>(BoundingBox::tile_y_to_merc(1362, BoundingBox::zoom_to_map_with(zoom))) == 6711782);
    }
}

TEST_CASE("Test coordinate conversion of BoundingBox class, zoom level 0") {
    int zoom = 0;

    SECTION("convert x index of tile ID to longitude, north-east of coordinate origin") {
        REQUIRE(static_cast<int>(BoundingBox::tile_x_to_merc(0, BoundingBox::zoom_to_map_with(zoom))) == -20037508);
    }

    SECTION("convert y index of tile ID to latitude, north-east of coordinate origin") {
        REQUIRE(static_cast<int>(BoundingBox::tile_y_to_merc(0, BoundingBox::zoom_to_map_with(zoom))) == 20037508);
    }
}

TEST_CASE("Test coordinate conversion of BoundingBox class, zoom level 1") {
    int zoom = 1;

    SECTION("convert x index of tile ID to longitude, north-east of coordinate origin") {
        REQUIRE(static_cast<int>(BoundingBox::tile_x_to_merc(1, BoundingBox::zoom_to_map_with(zoom))) == 0);
    }

    SECTION("convert y index of tile ID to latitude, north-east of coordinate origin") {
        REQUIRE(static_cast<int>(BoundingBox::tile_y_to_merc(1, BoundingBox::zoom_to_map_with(zoom))) == 0);
    }
}

TEST_CASE("Test zoom level to map width conversion") {
    SECTION("zoom level 0") {
        REQUIRE(BoundingBox::zoom_to_map_with(0) == 1);
    }

    SECTION("convert y index of tile ID to latitude, north-east of coordinate origin") {
        REQUIRE(BoundingBox::zoom_to_map_with(1) == 2);
    }

    SECTION("convert y index of tile ID to latitude, north-east of coordinate origin") {
        REQUIRE(BoundingBox::zoom_to_map_with(4) == 16);
    }
}

TEST_CASE("Test radians to degree conversion") {
    SECTION("0 degree") {
        REQUIRE(BoundingBox::radians_to_degree(0) == 0);
    }

    SECTION("convert y index of tile ID to latitude, north-east of coordinate origin") {
        REQUIRE(BoundingBox::radians_to_degree(osmium::geom::PI) == 180);
    }

    SECTION("convert y index of tile ID to latitude, north-east of coordinate origin") {
        REQUIRE(BoundingBox::radians_to_degree(2 * osmium::geom::PI) == 360);
    }

    SECTION("convert y index of tile ID to latitude, north-east of coordinate origin") {
        REQUIRE(BoundingBox::radians_to_degree(0.5 * osmium::geom::PI) == 90);
    }

    SECTION("convert y index of tile ID to latitude, north-east of coordinate origin") {
        REQUIRE(BoundingBox::radians_to_degree(-osmium::geom::PI) == -180);
    }
}

TEST_CASE("Test conversion in constructor of BoundingBox class") {
    SECTION("0 degree") {
        BoundingBox bbox (0, 0, 0);
        REQUIRE(test_utils::in_interval(bbox.m_min_lon, -180.0, -179.9999) == true);
        REQUIRE(test_utils::in_interval(bbox.m_max_lon, 179.999, 180.0) == true);
        REQUIRE(test_utils::in_interval(bbox.m_min_lat, -85.07, -85.05) == true);
        REQUIRE(test_utils::in_interval(bbox.m_max_lat, 85.05, 85.07) == true);
    }
}
