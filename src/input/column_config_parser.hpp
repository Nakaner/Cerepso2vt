/*
 * column_config_parser.hpp
 *
 *  Created on:  2018-08-17
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef COLUMN_CONFIG_PARSER_HPP_
#define COLUMN_CONFIG_PARSER_HPP_

#include <vector>

#include <postgres_drivers/columns.hpp>

#include "../vectortile_generator_config.hpp"
#include "../osm_data_table.hpp"

namespace input {

    enum class ColumnConfigFlag : char {
        LINEAR = 1,
        POLYGON = 2,
        NOCOLUMN = 3,
        DELETE = 4
    };

    enum class GeometryBits : char {
        NONE = 0,
        NODE = 1,
        WAY = 2
    };

    /**
     * class to parse Osm2pgsql style file
     */
    class ColumnConfigParser {

        std::string m_filename;

        VectortileGeneratorConfig& m_config;

        postgres_drivers::ColumnsVector m_point_columns;
        postgres_drivers::ColumnsVector m_line_columns;
        postgres_drivers::ColumnsVector m_polygon_columns;

        template <typename... TArgs>
        void add_column(postgres_drivers::ColumnsVector& vec, TArgs&&... args) {
            vec.emplace_back(std::forward<TArgs>(args)...);
        }

        std::vector<ColumnConfigFlag> parse_flags(const std::string& flags);

    public:
        ColumnConfigParser() = delete;

        ColumnConfigParser(VectortileGeneratorConfig& config);

        static postgres_drivers::ColumnType str_to_column_type(const std::string& type);

        void parse();

        postgres_drivers::ColumnsVector& point_columns();

        postgres_drivers::ColumnsVector& line_columns();

        postgres_drivers::ColumnsVector& polygon_columns();
    };

} // namespace input

#endif /* COLUMN_CONFIG_PARSER_HPP_ */
