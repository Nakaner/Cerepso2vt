/*
 * postgres_parser.hpp
 *
 *  Created on: 21.11.2016
 *      Author: Michael Reichert
 */

#ifndef SRC_POSTGRES_PARSER_HPP_
#define SRC_POSTGRES_PARSER_HPP_

#include "type_conversion.hpp"

template <typename TSingleElementType>
class PostgresParser {
protected:
    /**
     * string representation we got from the database
     */
    std::string& m_string_repr;

    /**
     * current position inside #m_string_repr
     */
    size_t m_current_position = 0;

public:
    PostgresParser(std::string& string_representation) :
        m_string_repr(string_representation) {};

    virtual ~PostgresParser() {}

    /**
     * Has the parser reached the end of the structure to be parsed?
     *
     * \returns True if we are at the end.
     */
    virtual bool has_next() = 0;

    /**
     * returns the next key value pair as a pair of strings
     *
     * Call #has_next() first!
     */
    virtual TSingleElementType get_next() = 0;
};


#endif /* SRC_POSTGRES_PARSER_HPP_ */
