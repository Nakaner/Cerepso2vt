/*
 * data_access.hpp
 *
 *  Created on:  2019-10-14
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_INPUT_OSM2PGSQL_DATA_ACCESS_HPP_
#define SRC_INPUT_OSM2PGSQL_DATA_ACCESS_HPP_

#include "../osm_data_table.hpp"
#include "column_config_parser.hpp"
#include "nodes_provider.hpp"

namespace input {

    class Osm2pgsqlDataAccess {

        static postgres_drivers::Column osm_id;
        static postgres_drivers::Column tags;
        static postgres_drivers::Column point_column;
        static postgres_drivers::Column way_column;
        static postgres_drivers::Column nodes;
        static postgres_drivers::Column members;

        /// reference to `untagged_nodes` table
        std::unique_ptr<input::NodesProvider> m_nodes_provider;
        /// reference to `planet_osm_line` table
        OSMDataTable m_line_table;
        /// reference to `planet_osm_ways` table
        OSMDataTable m_ways_table;
        /// reference to `planet_osm_polygon` table
        OSMDataTable m_polygon_table;
        /// reference to `planet_osm_rels` table
        OSMDataTable m_rels_table;

        osm_vector_tile_impl::slim_way_callback_type m_add_way_callback;
        osm_vector_tile_impl::slim_relation_callback_type m_add_relation_callback;

        /**
         * create all necessary prepared statements for this table
         *
         * This method chooses the suitable prepared statements which are dependend from the table type (point vs. way vs. …).
         * It overwrites the method of the superclass but calls the method of the superclass.
         *
         * \throws std::runtime_error
         */
        void create_prepared_statements();

        /**
         * \brief Throw std::runtime_error and use a message template receiving two parameters.
         *
         * The table name has to be the first template argument, the object ID the second argument
         * in the message template.
         *
         * \param templ template string
         * \param table_name table name
         * \param id OSM object ID
         */
        void throw_db_related_exception(const char* templ, const char* table_name, const osmium::object_id_type id);

        /**
         * Get a vector of pairs of strings from the a PostgreSQL string array.
         */
        std::vector<osm_vector_tile_impl::StringPair> tags_from_pg_string_array(std::string& tags_arr_str);

        /**
         * \brief Parse the response of the database after querying ways
         *
         * Does not clean up memory afterwards!
         *
         * \param result query result
         */
        void parse_way_query_result(PGresult* result);

        /**
         * \brief Get nodes and tags of a way from the planet_osm_ways table and call the callback
         * to create the OSM ways in the output file.
         *
         * \param id OSM way ID
         */
        void way_nodes_and_tags(osmium::object_id_type id);

        /**
         * \brief Parse the response of the database after querying relations
         *
         * Does not clean up memory afterwards!
         *
         * \param result query result
         */
        void parse_relation_query_result(PGresult* result);

        /**
         * \brief Get relation members and tags from the planet_osm_rels table and call the
         * callback to create the OSM relations in the output file.
         *
         * \param id OSM relation ID
         */
        void relation_tags_and_members(const osmium::object_id_type id);

        /**
         * Retrieve ways from planet_osm_line or planet_osm_polygon table (tags) and
         * planet_osm_ways (nodes) table.
         *
         * \param table table (planet_osm_line or planet_osm_polygon) to query
         * \param prepared_statement_name name of the prepared statement to use
         */
        void get_ways(OSMDataTable& table, const char* prepared_statement_name);

        /**
         * Factory method for OSMDataTable
         *
         * \param name name of the table
         * \param config programme config
         * \param core_columns columns which are always present such as osm_id and tags
         * \param additional_columns additional columns depending on database style
         */
        static OSMDataTable build_table(const char* name, VectortileGeneratorConfig& config,
                postgres_drivers::ColumnsVector&& core_columns,
                postgres_drivers::ColumnsVector* additional_columns = nullptr);

    public:
        Osm2pgsqlDataAccess(VectortileGeneratorConfig& config,
                input::ColumnConfigParser& column_config_parser);

        Osm2pgsqlDataAccess(Osm2pgsqlDataAccess&& other);

        void set_bbox(const BoundingBox& bbox);

        void set_add_node_callback(osm_vector_tile_impl::node_callback_type&& callback,
                osm_vector_tile_impl::node_without_tags_callback_type&& callback_without_tags,
                osm_vector_tile_impl::simple_node_callback_type&& simple_callback);

        void set_add_way_callback(osm_vector_tile_impl::way_callback_type&& callback,
                osm_vector_tile_impl::slim_way_callback_type&& slim_callback);

        void set_add_relation_callback(osm_vector_tile_impl::relation_callback_type&& callback,
                osm_vector_tile_impl::slim_relation_callback_type&& slim_callback);

        /**
         * \brief Get all nodes in the tile
         *
         * \throws std::runtime_error
         */
        void get_nodes_inside();

        /**
         * \brief Get all missing nodes
         *
         * \param missing_nodes nodes to fetch from the database
         */
        void get_missing_nodes(const osm_vector_tile_impl::osm_id_set_type& missing_nodes);

        /**
         * \brief Get all ways inside the tile
         */
        void get_ways_inside();

        /**
         * \brief Get all missing ways
         *
         * \param missing_ways ways to fetch from the database
         */
        void get_missing_ways(const osm_vector_tile_impl::osm_id_set_type& m_missing_ways);

        /**
         * \brief Get all relations inside the tile
         */
        void get_relations_inside();

        /**
         * \brief Get all missing relations
         *
         * \param missing_relations relations to fetch from the database
         */
        void get_missing_relations(const osm_vector_tile_impl::osm_id_set_type& missing_relations);
    };
}



#endif /* SRC_INPUT_OSM2PGSQL_DATA_ACCESS_HPP_ */
