/*
 * hstore_parser.hpp
 *
 *  Created on: 2016-10-13
 *      Author: Michael Reichert
 */

#ifndef SRC_HSTORE_PARSER_HPP_
#define SRC_HSTORE_PARSER_HPP_

#include <assert.h>
#include "postgres_parser.hpp"

using HStorePartsType = char;

/**
 * \brief track where we are during parsing hstore
 */
enum class HStoreParts : HStorePartsType {
    NONE = 0,
    KEY = 1,
    SEPARATOR = 2,
    VALUE = 3,
    END = 4
};

using StringPair = std::pair<std::string, std::string>;

/**
 * \brief Parser class for hstores encoded as strings
 *
 * See the [documentation](https://www.postgresql.org/docs/current/static/hstore.html)
 * of PostgreSQL for the representation of hstores as strings.
 *
 * Keys and values are always strings.
 *
 */
class HStoreParser : public PostgresParser<StringPair> {

    /// shortcut
    using postgres_parser_type = PostgresParser<StringPair>;

    /**
     * \brief Minimum length of a key value pair.
     *
     * Both key and value must be longer than 1 character each. The key-value separator needs two
     * characters. Quotation marks have to be added. Example: "k"=>"v"
     *
     * This results to 7 (left quotation mark does not count).
     *
     * We don't need to start parsing if m_current_position > m_string_repr.size() - MIN_KV_PAIR_LENGTH.
     */
    const size_t MIN_KV_PAIR_LENGTH = 7;

    /// track progress of hstore parsing
    HStoreParts m_parse_progress = HStoreParts::NONE;

    /// buffer for the characters of the key during its parsing
    std::string m_key_buffer;

    /// buffer for the characters of the value during its parsing
    std::string m_value_buffer;

    /**
     * \brief Add a character to either the key or the value buffer.
     *
     * It depends on the value of #m_parse_progress wether the character will be added to the key or the value buffer.
     *
     * \param character the character to be added
     *
     * \throws std::runtime_error if #m_parse_progress is neither HStoreParts::KEY nor HStoreParts::VALUE
     */
    void add_to_key_or_value(char character);

    /**
     * \brief reset key and value buffer
     */
    void reset_buffers();

    /**
     * \brief increase progress counter
     *
     * This helper method is used to shorten the long if-then-else constructs in get_next().
     */
    void increase_parse_progress();

    /**
     * \brief Throw std::runtime_error due to invalid hstore syntax
     *
     * \param error error type to be included into the message
     *
     * \throws std::runtime_error
     */
    void invalid_hstore_syntax(std::string error);

public:
    /**
     * \param string_repr string representation of the hstore
     */
    HStoreParser(std::string& string_repr) : PostgresParser<StringPair>(string_repr) {};

    /**
     * has the parser reached the end of the hstore
     */
    bool has_next();

    /**
     * \brief Returns the next key value pair as a pair of strings.
     *
     * \throws std::runtime_error if parsing fails due to a syntax error
     */
    StringPair get_next();
};


#endif /* SRC_HSTORE_PARSER_HPP_ */
