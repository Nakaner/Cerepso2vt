/*
 * column_config_parser.cpp
 *
 *  Created on:  2018-08-17
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "column_config_parser.hpp"

using GeometryBitsBaseType = std::underlying_type<input::GeometryBits>::type;

constexpr input::GeometryBits operator|(const input::GeometryBits lhs, const input::GeometryBits rhs) noexcept {
    return static_cast<input::GeometryBits>(static_cast<GeometryBitsBaseType>(lhs) | static_cast<GeometryBitsBaseType>(rhs));
}

constexpr input::GeometryBits operator&(const input::GeometryBits lhs, const input::GeometryBits rhs) noexcept {
    return static_cast<input::GeometryBits>(static_cast<GeometryBitsBaseType>(lhs) & static_cast<GeometryBitsBaseType>(rhs));
}

input::ColumnConfigParser::ColumnConfigParser(VectortileGeneratorConfig& config) :
    m_config(config),
    m_point_columns(),
    m_line_columns(),
    m_polygon_columns() {
}

std::vector<input::ColumnConfigFlag> input::ColumnConfigParser::parse_flags(const std::string& flags) {
    size_t pos = 0;
    size_t next = flags.find_first_of(",", pos);
    std::vector <ColumnConfigFlag> vec;
    while (true) {
        if (flags.substr(pos, next - pos) == "nocolumn") {
            vec.push_back(ColumnConfigFlag::NOCOLUMN);
        } else if (flags.substr(pos, next - pos) == "delete") {
            vec.push_back(ColumnConfigFlag::DELETE);
        }
        if (next == std::string::npos) {
            break;
        }
        pos = next + 1;
        next = flags.find_first_of(",", pos);
    }
    return vec;
}

/*static*/ postgres_drivers::ColumnType input::ColumnConfigParser::str_to_column_type(const std::string& type) {
    if (type == "text") {
        return postgres_drivers::ColumnType::TEXT;
    } else if (type == "int4") {
        return postgres_drivers::ColumnType::INT;
    } else if (type == "real") {
        return postgres_drivers::ColumnType::REAL;
    }
    return postgres_drivers::ColumnType::NONE;
}

void input::ColumnConfigParser::parse() {
    if (m_config.m_osm2pgsql_style.empty()) {
        return;
    }
    std::ifstream csv_read;
    csv_read.open(m_config.m_osm2pgsql_style, std::ios::in);
    if (!csv_read.good()) {
        throw std::runtime_error{"Open file " + m_filename + " failed."};
    }
    std::string line;
    while(std::getline(csv_read, line)) {
        // delete comments
        if (line.find("#") != std::string::npos) {
            line.erase(line.begin() + line.find("#"), line.end());
        }
        // omit empty lines
        if (line.size() == 0){
            continue;
        }

        // geometry is not used here
        std::string geometry;
        std::string name;
        std::string type;
        std::string flags;
        size_t i = 0;
        size_t pos = 0;
        size_t next = line.find_first_of(" ", pos);
        bool create_column = true; // Has a column to be created from this line?
        while (i < 4) {
            if (next == pos) {
                pos = next + 1;
                next = line.find_first_of(" ", pos);
                continue;
            }
            ++i;
            if (i == 1 && pos == 0) {
                geometry = line.substr(0, next - pos);
            } else if (i == 1) {
                geometry = line.substr(pos, next - pos);
            } else if (i == 2) {
                name = line.substr(pos, next - pos);
            } else if (i == 3) {
                type = line.substr(pos, next - pos);
            } else if (i == 4) {
                flags = line.substr(pos, next - pos);
            }
            if (next == std::string::npos) {
                break;
            }
            pos = next + 1;
            next = line.find_first_of(" ", pos);
        }


        GeometryBits geom_type = GeometryBits::NONE;

        if (geometry == "node") {
            geom_type = GeometryBits::NODE;
        } else if (geometry == "way") {
            geom_type = GeometryBits::WAY;
        } else if (geometry == "node,way" || geometry == "way,node") {
            geom_type = (GeometryBits::NODE | GeometryBits::WAY);
        } else {
            throw std::runtime_error("Unsupported geometry type");
        }
        if (name == "way_area" || name == "z_order") {
            continue;
        }
        postgres_drivers::ColumnType ctype = str_to_column_type(type);
        if (ctype == postgres_drivers::ColumnType::NONE) {
            throw std::runtime_error("Unsupported column type");
        }
        std::vector<ColumnConfigFlag> flags_vec = parse_flags(flags);
        for (auto f : flags_vec) {
            if (f == ColumnConfigFlag::DELETE) {
                create_column = false;
            } else if (f == ColumnConfigFlag::NOCOLUMN) {
                create_column = false;
            }
        }
        if (name.empty()) {
            create_column = false;
        }
        if (!create_column) {
            // We can skip the following because no column has to be created from this line.
            continue;
        }
        if ((geom_type & GeometryBits::NODE) == GeometryBits::NODE) {
            m_point_columns.emplace_back(name, ctype, postgres_drivers::ColumnClass::TAG);
        }
        if ((geom_type & GeometryBits::WAY) == GeometryBits::WAY) {
            m_line_columns.emplace_back(name, ctype, postgres_drivers::ColumnClass::TAG);
            m_polygon_columns.emplace_back(name, ctype, postgres_drivers::ColumnClass::TAG);
        }
    }
}

postgres_drivers::ColumnsVector& input::ColumnConfigParser::point_columns() {
    return m_point_columns;
}

postgres_drivers::ColumnsVector& input::ColumnConfigParser::line_columns() {
    return m_line_columns;
}

postgres_drivers::ColumnsVector& input::ColumnConfigParser::polygon_columns() {
    return m_polygon_columns;
}
