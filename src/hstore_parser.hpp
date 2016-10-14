/*
 * hstore_parser.hpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */

#ifndef SRC_HSTORE_PARSER_HPP_
#define SRC_HSTORE_PARSER_HPP_

using StringPair = std::pair<std::string, std::string>;

class HStoreParser {
private:
    std::string& m_hstore;
    size_t m_current_position = 0;

public:
    HStoreParser(std::string& hstore);

    /**
     * has the parser reached the end of the hstore
     */
    bool has_next();

    /**
     * returns the next key value pair as a pair of strings
     */
    StringPair get_next();
};


#endif /* SRC_HSTORE_PARSER_HPP_ */
