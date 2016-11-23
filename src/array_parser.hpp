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
        if (postgres_parser_type::m_string_repr.at(m_current_position) != '}') {
            return true;
        }
        return false;
    }

    /**
     * returns the next key value pair as a pair of strings
     */
    typename TypeConversion::output_type get_next() {
        std::string to_convert;
        bool inside_quotation_marks = false; // track if we are inside '""
        if (m_current_position == 1 && postgres_parser_type::m_string_repr.at(m_current_position) == '"') {
            inside_quotation_marks = true;
            m_current_position++;
        }
        while (!element_finished(postgres_parser_type::m_string_repr.at(m_current_position), inside_quotation_marks)) {
            to_convert.push_back(postgres_parser_type::m_string_repr.at(m_current_position));
            if (inside_quotation_marks && (postgres_parser_type::m_string_repr.at(m_current_position) == '"')) {

            }
            if (postgres_parser_type::m_string_repr.at(m_current_position) == '"') {
                inside_quotation_marks = true;
            }
            m_current_position++;
        }
        if (inside_quotation_marks) {
            // If we reached the end of the element because it was surrounded by quotation marks, we are now sitting "on"
            // the quotation mark. We have to move one character forward to reach the comma.
            m_current_position++;
            // Now we are at the comma separating the entries.
        }
        // move m_current_position to beginning of next element
        m_current_position++;
        return m_type_conversion.to_output_format(to_convert);
    }
};

#endif /* SRC_ARRAY_PARSER_HPP_ */
