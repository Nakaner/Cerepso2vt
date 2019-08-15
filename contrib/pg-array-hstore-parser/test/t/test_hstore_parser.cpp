/*
 * test_hstore_parser.cpp
 *
 *  Created on: 2016-11-22
 *      Author: Michael Reichert
 */

#include "catch.hpp"
#include "util.hpp"
#include <hstore_parser.hpp>

TEST_CASE( "Test hstore parser") {
    std::string string_repr;

    SECTION("two key-value pairs") {
        string_repr = R"("ref"=>"7", "is_in"=>"Bezirk Laufenburg,Aargau,Schweiz,Europe")";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("ref", "7"), std::make_pair("is_in", "Bezirk Laufenburg,Aargau,Schweiz,Europe")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("escaping of quotation marks") {
        string_repr = R"("foo\"bar"=>"baz")";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("foo\"bar", "baz")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("escaping of backslashes") {
        string_repr = R"("foo\\bar"=>"baz")";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("foo\\bar", "baz")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("escaping of quotation marks at the beginning of a key") {
        string_repr = R"("\"hello"=>"mike")";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("\"hello", "mike")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("escaping of backslashes at the beginning of a key") {
        string_repr = R"("\\hello"=>"george")";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("\\hello", "george")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("escaping of quotation marks at the end of a key") {
        string_repr = R"("goodbye\""=>"kate")";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("goodbye\"", "kate")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("escaping of quotation marks at the beginning of a value") {
        string_repr = R"("hello"=>"\"mike")";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("hello", "\"mike")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("escaping of backslashes at the beginning of a value") {
        string_repr = R"("hello"=>"\\george")";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("hello", "\\george")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("escaping of quotation marks at the end of a value") {
        string_repr = R"("goodbye"=>"kate\"")";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("goodbye", "kate\"")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("key and value only containing backslashes and quotation marks") {
        string_repr = R"("\\\\"=>"\"\"")";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("\\\\", "\"\"")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("key without surrounding quotation marks") {
        string_repr = R"(abc=>"def")";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("abc","def")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("keys without surrounding quotation marks") {
        string_repr = R"(abc=>"def",foo=>"any")";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("abc","def"), std::make_pair("foo", "any")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("values without surrounding quotation marks") {
        string_repr = R"("abc"=>def,"foo"=>any)";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("abc","def"), std::make_pair("foo", "any")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("keys and values without surrounding quotation marks") {
        string_repr = R"(abc=>def,foo=>any)";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("abc","def"), std::make_pair("foo", "any")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("keys and values without surrounding quotation marks but with spaces around the first =>") {
        string_repr = R"(abc => def,foo=>any)";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("abc","def"), std::make_pair("foo", "any")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("keys and values without surrounding quotation marks but with spaces around the comma") {
        string_repr = R"(abc => def , foo=>any)";
        std::vector<pg_array_hstore_parser::StringPair> expected {std::make_pair("abc","def"), std::make_pair("foo", "any")};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }

    SECTION("empty hstore") {
        string_repr = "";
        std::vector<pg_array_hstore_parser::StringPair> expected {};
        pg_array_hstore_parser::HStoreParser hstore_parser (string_repr);
        std::vector<pg_array_hstore_parser::StringPair> got;
        while (hstore_parser.has_next()) {
            got.push_back(hstore_parser.get_next());
        }
        REQUIRE(test_utils::compare_vectors(got, expected) == true);
    }
}

