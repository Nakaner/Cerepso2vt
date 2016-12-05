/*
 * test_item_type_conversion.hpp
 *
 *  Created on:  2016-12-05
 *      Author: Michael Reichert
 */

#include "catch.hpp"
#include <item_type_conversion.hpp>

TEST_CASE("Test item type conversion") {
    std::string string_repr;
    ItemTypeConversion item_type_conv;

    SECTION("node") {
        string_repr = "n";
        REQUIRE(item_type_conv.to_output_format(string_repr) == osmium::item_type::node);
    }

    SECTION("way") {
        string_repr = "w";
        REQUIRE(item_type_conv.to_output_format(string_repr) == osmium::item_type::way);
    }

    SECTION("relation") {
        string_repr = "r";
        REQUIRE(item_type_conv.to_output_format(string_repr) == osmium::item_type::relation);
    }

    SECTION("unsupported type") {
        string_repr = "c";
        REQUIRE(item_type_conv.to_output_format(string_repr) == osmium::item_type::undefined);
    }

    SECTION("unsupported type") {
        string_repr = "c8hfgh";
        REQUIRE_THROWS_AS(item_type_conv.to_output_format(string_repr), std::runtime_error&);
    }
}
