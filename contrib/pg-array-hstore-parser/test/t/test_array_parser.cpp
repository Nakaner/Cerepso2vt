/*
 * test_array_parser.cpp
 *
 *  Created on: 21.11.2016
 *      Author: Michael Reichert
 */

#include "catch.hpp"
#include "util.hpp"
#include <array_parser.hpp>

TEST_CASE("Test array parsing") {
    std::string string_repr;

    SECTION("array of integers") {
        string_repr = "{1,5,8,65,75,1514}";
        int expected_array[] {1,5,8,65,75,1514};
        std::vector<int64_t> expected (expected_array, expected_array + sizeof(expected_array) / sizeof(int));
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::Int64Conversion> array_parser (string_repr);
        std::vector<int64_t> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("array of integers containing spaces") {
        string_repr = "{1 , 5, 8,65, 75 ,1514 }";
        int expected_array[] {1,5,8,65,75,1514};
        std::vector<int64_t> expected (expected_array, expected_array + sizeof(expected_array) / sizeof(int));
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::Int64Conversion> array_parser (string_repr);
        std::vector<int64_t> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("array of integers containing spaces, one is NULL") {
        string_repr = "{1 , 5, 8, NULL, 75 ,1514 }";
        int expected_array[] {1,5,8,0,75,1514};
        std::vector<int64_t> expected (expected_array, expected_array + sizeof(expected_array) / sizeof(int));
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::Int64Conversion> array_parser (string_repr);
        std::vector<int64_t> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("array of strings") {
        string_repr = R"({"ab", foobar})";
        std::vector<std::string> expected;
        expected.push_back("ab");
        expected.push_back("foobar");
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> array_parser (string_repr);
        std::vector<std::string> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("array of strings, one is empty") {
        string_repr = R"({"ab", "", "any"})";
        std::vector<std::string> expected;
        expected.push_back("ab");
        expected.push_back("");
        expected.push_back("any");
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> array_parser (string_repr);
        std::vector<std::string> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("array of strings, one is NULL") {
        string_repr = R"({"ab", "any", NULL})";
        std::vector<std::string> expected;
        expected.push_back("ab");
        expected.push_back("any");
        expected.push_back("");
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> array_parser (string_repr);
        std::vector<std::string> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("array of strings, one is the word \"NULL\"") {
        string_repr = R"({"ab", "any", "NULL"})";
        std::vector<std::string> expected;
        expected.push_back("ab");
        expected.push_back("any");
        expected.push_back("NULL");
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> array_parser (string_repr);
        std::vector<std::string> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("array of strings, one contains a closing curly brace") {
        string_repr = R"({"ab", "an}y", "ham"})";
        std::vector<std::string> expected;
        expected.push_back("ab");
        expected.push_back("an}y");
        expected.push_back("ham");
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> array_parser (string_repr);
        std::vector<std::string> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("array of strings, one contains an opening curly brace") {
        string_repr = R"({"ab", "an{y", "ham"})";
        std::vector<std::string> expected;
        expected.push_back("ab");
        expected.push_back("an{y");
        expected.push_back("ham");
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> array_parser (string_repr);
        std::vector<std::string> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("array with only one string containing an escaped double quote") {
        string_repr = R"({"an\"y"})";
        std::vector<std::string> expected;
        expected.push_back("an\"y");
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> array_parser (string_repr);
        std::vector<std::string> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("array with only one string containing two escaped double quote") {
        string_repr = R"({"He spoke: \"Any 'star' in the sky is yours.\""})";
        std::vector<std::string> expected;
        expected.push_back("He spoke: \"Any 'star' in the sky is yours.\"");
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> array_parser (string_repr);
        std::vector<std::string> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("array of strings, one contains an escaped double quote") {
        string_repr = R"({"ab", "an\"y", "ham"})";
        std::vector<std::string> expected;
        expected.push_back("ab");
        expected.push_back("an\"y");
        expected.push_back("ham");
        pg_array_hstore_parser::ArrayParser<pg_array_hstore_parser::StringConversion> array_parser (string_repr);
        std::vector<std::string> got;
        while (array_parser.has_next()) {
            got.push_back(array_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }
}
