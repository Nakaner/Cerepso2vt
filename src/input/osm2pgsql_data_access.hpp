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
         * \brief Get IDs of objects in a given database table in the bounding box
         *
         * \param table database table to query (usually line or polygon)
         * \param prepared_statement_name name of the prepared statement to execute
         *
         * \returns vector of way IDs in the bounding box
         */
        std::vector<osmium::object_id_type> get_ids_inside(OSMDataTable& table, const char* prepared_statement_name);

        /**
         * \brief Get nodes and tags of a way from the planet_osm_ways table and call the callback
         * to create the OSM ways in the output file.
         *
         * \param id_list list of OSM IDs to query separated by comma to be inserted into the SQL statement
         */
        void query_and_flush_ways(char* id_list);

        /**
         * \brief Parse the response of the database after querying relations
         *
         * Does not clean up memory afterwards!
         *
         * \param result query result
         */
        void parse_relation_query_result(PGresult* result);

        /**
         * \brief Iterate over a container of OSM object IDs and call a callback function for each batch of IDs.
         *
         * This method joins the IDs to a comma separated list of IDs as template argument for a
         * prepared SQL statement and calls the callback function with this list. Each list consists of a batch
         * of up to 100 IDs.
         *
         * \param begin iterator pointing to the first element in the input container
         * \param end iterator pointing to the first element after the range
         * \param fields fields to query (comma separated list of field names, will be inserted
         *        into the SQL query directly without escaping)
         * \param callback callback to be called
         *
         * \tparam TIterator iterator of a container of osmium::object_id_type
         */
        template <typename TIterator>
        void get_objects_from_db(TIterator begin, TIterator end, const char* fields,
                const char* table_name, std::function<void(char*)> callback) {
            constexpr unsigned long int batch_size = 100;
            constexpr unsigned long int number_width = 25;
            constexpr unsigned long int str_maxlen = number_width * batch_size + 1;
            char sql[str_maxlen];
            const char* sql_template = "SELECT %s FROM %s WHERE id IN (";
            int printed = snprintf(sql, str_maxlen - 26, sql_template, fields, table_name);
            char* sql_ptr = sql + printed;
            size_t rel_count = 0;
            for (auto it = begin; it != end; ++it) {
                int printed_chars = 0;
                if (rel_count > 0) {
                    printed_chars = sprintf(sql_ptr, ",%ld", *it);
                } else {
                    printed_chars = sprintf(sql_ptr, "%ld", *it);
                }
                sql_ptr = sql_ptr + printed_chars;
                ++rel_count;
                if (rel_count == batch_size || str_maxlen - (sql_ptr - sql) - 3 < number_width) {
                    sprintf(sql_ptr, ");");
                    callback(sql);
                    rel_count = 0;
                    printed_chars = snprintf(sql, str_maxlen - 26, sql_template, fields, table_name);
                    sql_ptr = sql + printed_chars;
                }
            }
            if (rel_count > 0) {
                sprintf(sql_ptr, ");");
                callback(sql);
            }
        }

        /**
         * \brief Get relation members and tags from the planet_osm_rels table and call the
         * callback to create the OSM relations in the output file.
         *
         * \param id_list list of OSM relation IDs
         */
        void query_and_flush_relations(char* id_list);

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
         *
         * \returns vector of found OSM relation IDs
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
