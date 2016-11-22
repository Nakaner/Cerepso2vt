/*
 * hstore_parser.cpp
 *
 *  Created on:  2016-11-22
 *      Author: Michael Reichert
 */

#include <stdexcept>
#include "hstore_parser.hpp"

/**
 * allows usage of ++HStoreParts
 */
HStoreParts& operator++(HStoreParts& progress) {
    progress = static_cast<HStoreParts>(static_cast<HStorePartsType>(progress) + 1);
    assert(progress <= HStoreParts::END);
    return progress;
}

void HStoreParser::increase_parse_progress() {
    ++m_parse_progress;
}

void HStoreParser::add_to_key_or_value(char character) {
    switch (m_parse_progress) {
    case HStoreParts::KEY :
        m_key_buffer.push_back(character);
        break;
    case HStoreParts::VALUE :
        m_value_buffer.push_back(character);
        break;
    default :
        assert(false && "You should call add_key_to_value(char) only if you are reading a character of a key or of a value!");
    }
}

void HStoreParser::reset_buffers() {
    m_key_buffer.clear();
    m_value_buffer.clear();
}

void HStoreParser::invalid_hstore_syntax(std::string error) {
    std::string message = "Invalid hstore syntax at character ";
    message += std::to_string(m_current_position + 1);
    message += " of \"";
    message += m_string_repr;
    message += "\". \"\\";
    message += m_string_repr.at(m_current_position);
    message += "\" ";
    message += error;
    message += '\n';
    throw std::runtime_error(message);
}

bool HStoreParser::has_next() {
    if (m_current_position > m_string_repr.size() - MIN_KV_PAIR_LENGTH) {
        return false;
    }
    return true;
}

StringPair HStoreParser::get_next() {
    reset_buffers();
    m_parse_progress = HStoreParts::NONE;
    char accumulated_backslashes = 0; // counts preceding backslashes
    bool quoted_string = false; // track if the key/value is surrounded by quotation marks

    while (m_current_position < m_string_repr.size() && m_parse_progress != HStoreParts::END) {
        /// Only two special characters have to be escaped in hstores (not more): quotation marks and backslashes.
        /// All other characters inside keys and values do not need any escaping.

        // Handling of escaped quotation marks and backslashes
        if (accumulated_backslashes == 1) {
            switch (m_string_repr.at(m_current_position)) {
            case '"':
                add_to_key_or_value('"');
                accumulated_backslashes = 0;
                break;
            case '\\':
                add_to_key_or_value('\\');
                accumulated_backslashes = 0;
                break;
            default:
                invalid_hstore_syntax("is no valid escape sequence in a hstore key or value.");
            }
        } else if (m_string_repr.at(m_current_position) == '\\') {
            accumulated_backslashes++;

        // Handling of all other characters
        } else if (m_string_repr.at(m_current_position) == '=') {
            // Key/Values must be surrounded by quotation marks if they contain a '='.
            if (!quoted_string && (m_parse_progress == HStoreParts::KEY)) {
                increase_parse_progress();
            } else if (!quoted_string && (m_parse_progress == HStoreParts::VALUE)) {
                invalid_hstore_syntax("\'=\' is not allowed at the end of a value.");
            } else if (quoted_string && (m_parse_progress == HStoreParts::KEY || m_parse_progress == HStoreParts::VALUE)) {
                add_to_key_or_value(m_string_repr.at(m_current_position));
            } else if (m_parse_progress == HStoreParts::NONE) {
                invalid_hstore_syntax("\'=\' is not allowed there.");
            }
        } else if (m_string_repr.at(m_current_position) == '>') {
            // Key/Values must be surrounded by quotation marks if they contain a '>'.
            if (!quoted_string && (m_parse_progress == HStoreParts::KEY || m_parse_progress == HStoreParts::VALUE)) {
                invalid_hstore_syntax("\'>\' is not allowed inside a key/value or at its end without a preceding '='.");
            } else if (quoted_string && (m_parse_progress == HStoreParts::KEY || m_parse_progress == HStoreParts::VALUE)) {
                add_to_key_or_value(m_string_repr.at(m_current_position));
            } else if (m_parse_progress != HStoreParts::SEPARATOR) {
                invalid_hstore_syntax("\'>\' is not allowed there.");
            }
        } else if (m_string_repr.at(m_current_position) == '"') {
            switch (m_parse_progress) {
            case HStoreParts::NONE :
            case HStoreParts::SEPARATOR :
                increase_parse_progress();
                quoted_string = true;
                break;
            case HStoreParts::KEY :
            case HStoreParts::VALUE :
                increase_parse_progress();
                quoted_string = false;
                break;
            case HStoreParts::END :
                invalid_hstore_syntax("double '\"' inserted.");
            }
        } else if (m_string_repr.at(m_current_position) == ' ' || m_string_repr.at(m_current_position) == ',') {
            /// If a space or a comma appears in a key or value, the key/value has to be surrounded by quotation marks.
            if (m_parse_progress == HStoreParts::KEY || m_parse_progress == HStoreParts::VALUE) {
                if (quoted_string) {
                    add_to_key_or_value(m_string_repr.at(m_current_position));
                } else {
                    increase_parse_progress();
                }
            }
        } else {
            // We only reach this branch if the current character is none of the following: comma, space, quotation mark,
            // =, >
            switch (m_parse_progress) {
            case HStoreParts::KEY :
            case HStoreParts::VALUE :
                add_to_key_or_value(m_string_repr.at(m_current_position));
                break;
            case HStoreParts::NONE :
            case HStoreParts::SEPARATOR :
                // There may be spaces before a key or between "=>" and the begin of a value.
                increase_parse_progress();
                add_to_key_or_value(m_string_repr.at(m_current_position));
                break;
            default :
                break;
            }
        }
        m_current_position++;
    }
    return std::make_pair(m_key_buffer, m_value_buffer);
}
