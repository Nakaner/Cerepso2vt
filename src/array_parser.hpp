/*
 * array_parser.hpp
 *
 *  Created on: 2016-11-21
 *      Author: Michael Reichert
 */

#ifndef SRC_ARRAY_PARSER_HPP_
#define SRC_ARRAY_PARSER_HPP_

#include <assert.h>
#include "type_conversion.hpp"
#include "postgres_parser.hpp"

using ArrayPartsType = char;

/**
 * \brief track where we are during parsing an array
 */
enum class ArrayParts : ArrayPartsType {
    NONE = 0,
    ELEMENT = 1,
    END = 2
};

/**
 * \brief Class to parse one dimensional arrays received from a PostgreSQL database query response which are encoded as strings.
 *
 * This class needs a type conversion implementation as template argument. Use the aliases defined by type_conversion.hpp.
 *
 * See tests/t/test_array_parser.cpp for usage examples of this class.
 */
template <typename TypeConversion>
class ArrayParser : public PostgresParser<typename TypeConversion::output_type> {

    /// shortcut
    using postgres_parser_type = PostgresParser<typename TypeConversion::output_type>;

    /// track progress of hstore parsing
    ArrayParts m_parse_progress = ArrayParts::NONE;

    TypeConversion m_type_conversion;

    /**
     * Current position inside #m_string_repr
     *
     * We always start parsing at the second character (i.e. index 1).
     */
    size_t m_current_position = 1;

    /**
     * Check if we have reached the end of the element of an arry.
     *
     * \param next_char next character of the string
     * \param inside_quotation_marks true if the character before was inside a "" section; false otherwise
     *
     * \returns true if next_char does not belong to an element of the array any more (i.e. ',' outside
     *          '""' or '}' outside '""').
     */
    bool element_finished(const char next_char, const bool inside_quotation_marks) {
        if (inside_quotation_marks && next_char == '"') {
            return true;
        } else if (!inside_quotation_marks && next_char == ',') {
            return true;
        } else if (!inside_quotation_marks && next_char == '}') {
            return true;
        }
        return false;
    }

    void invalid_syntax(std::string error) {
        std::string message = "Invalid array syntax at character ";
        message += std::to_string(m_current_position + 1);
        message += " of \"";
        message += postgres_parser_type::m_string_repr;
        message += "\". \"\\";
        message += postgres_parser_type::m_string_repr.at(m_current_position);
        message += "\" ";
        message += error;
        message += '\n';
        throw std::runtime_error(message);
    }

public:
    ArrayParser(std::string& string_repr) : PostgresParser<typename TypeConversion::output_type>(string_repr) {};

    /**
     * \brief Has the parser reached the end of the hstore?
     */
    bool has_next() {
        if (m_current_position >= postgres_parser_type::m_string_repr.size() - 1) {
            // We have reached the last character which is a '}'.
            return false;
        }
        if (postgres_parser_type::m_string_repr.at(m_current_position) == '}') {
            return false;
        }
        return true;
    }

    /**
     * \brief Return the next key value pair as a pair of strings.
     *
     * Parsing follows following rules of the Postgres documentation:
     * The array output routine will put double quotes around element values if they are empty strings,
     * contain curly braces, delimiter characters, double quotes, backslashes, or white space, or match
     * the word NULL. Double quotes and backslashes embedded in element values will be backslash-escaped.
     */
    typename TypeConversion::output_type get_next() {
        std::string to_convert;
        m_parse_progress = ArrayParts::NONE;
        bool backslashes_before = false; // counts preceding backslashes
        bool quoted_element = false; // track if the key/value is surrounded by quotation marks
        while (m_parse_progress != ArrayParts::END && m_current_position < postgres_parser_type::m_string_repr.size()) {
            if (m_parse_progress == ArrayParts::NONE) {
                if (postgres_parser_type::m_string_repr.at(m_current_position) == '"') {
                    quoted_element = true;
                    m_parse_progress = ArrayParts::ELEMENT;
                } else if (postgres_parser_type::m_string_repr.at(m_current_position) != ','
                        && postgres_parser_type::m_string_repr.at(m_current_position) != ' '){
                    m_parse_progress = ArrayParts::ELEMENT;
                    to_convert.push_back(postgres_parser_type::m_string_repr.at(m_current_position));
                }
            } else if (m_parse_progress == ArrayParts::ELEMENT && quoted_element) {
                // Handling of array elements surrounded by double quotes
                if (backslashes_before) {
                    // Handling of escaped characters. Elements containing escape sequences must be surrounded by double quotes.
                    switch (postgres_parser_type::m_string_repr.at(m_current_position)) {
                    case '"':
                        to_convert.push_back('"');
                        backslashes_before = false;
                        break;
                    case '\\':
                        to_convert.push_back('\\');
                        backslashes_before = false;
                        break;
                    default:
                        invalid_syntax("is no valid escape sequence in a hstore key or value.");
                    }
                } else if (postgres_parser_type::m_string_repr.at(m_current_position) == '\\') {
                    backslashes_before = true;
                } else if (postgres_parser_type::m_string_repr.at(m_current_position) == '"') {
                    m_parse_progress = ArrayParts::END;
                } else {
                    to_convert.push_back(postgres_parser_type::m_string_repr.at(m_current_position));
                }
            } else if (m_parse_progress == ArrayParts::ELEMENT && !quoted_element) {
                // Handling of array elements not surrounded by double quotes
                switch (postgres_parser_type::m_string_repr.at(m_current_position)) {
                case ' ':
                    // We will go on until we reach the comma. Therefore, we ignore this space and don't report a syntax error
                    // if there is a space in an array element which is not surrounded by double quotes.
                    break;
                case ',':
                case '}':
                    m_parse_progress = ArrayParts::END;
                    break;
                default:
                    to_convert.push_back(postgres_parser_type::m_string_repr.at(m_current_position));
                }
            }
            m_current_position++;
        }
        if (!quoted_element && to_convert == "NULL") {
            return m_type_conversion.return_null_value();
        }
        return m_type_conversion.to_output_format(to_convert);
    }
};

#endif /* SRC_ARRAY_PARSER_HPP_ */
