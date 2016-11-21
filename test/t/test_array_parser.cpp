/*
 * test_array_parser.cpp
 *
 *  Created on: 21.11.2016
 *      Author: Michael Reichert
 */

#include "catch.hpp"
#include "util.hpp"
#include <array_parser.hpp>

TEST_CASE( "Test array parsing") {
    std::string string_repr;

    SECTION("array of integers") {
        string_repr = "{1,5,8,65,75,1514}";
        int expected_array[] {1,5,8,65,75,1514};
        std::vector<int64_t> expected (expected_array, expected_array + sizeof(expected_array) / sizeof(int));
        ArrayParser<Int64Conversion> array_parser (string_repr);
        std::vector<int64_t> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }
}
