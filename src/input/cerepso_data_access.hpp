/*
 * crepso_data_access.hpp
 *
 *  Created on:  2019-08-19
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_CEREPSO_DATA_ACCESS_HPP_
#define SRC_CEREPSO_DATA_ACCESS_HPP_

#include <libpq-fe.h>
#include <functional>
#include <string>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>
#include <postgres_drivers/table.hpp>
#include "nodes_provider.hpp"
#include "metadata_fields.hpp"
#include "../osm_data_table.hpp"
#include "../osm_vector_tile_impl_definitions.hpp"
#include "../vectortile_generator_config.hpp"

namespace input {

    /**
     * Retrieve nodes, ways and relations from database tables imported with Cerepso and call the
     * callbacks of OSMVectorTileImpl to add the OSM objects to the output file.
     */
    class CerepsoDataAccess {

        VectortileGeneratorConfig& m_config;

        /// providing `nodes` and `untagged_nodes` table
        std::unique_ptr<input::NodesProvider> m_nodes_provider;
        /// `ways` table
        OSMDataTable m_ways_table;
        /// `relations` table
        OSMDataTable m_relations_table;
        /// `node_ways` table
        OSMDataTable m_node_ways_table;
        /// `node_relations` table
        OSMDataTable m_node_relations_table;
        /// way_relations` table
        OSMDataTable m_way_relations_table;
        /// `relation_relations` table
        OSMDataTable m_relation_relations_table;

        input::MetadataFields m_metadata_fields;

        osm_vector_tile_impl::node_callback_type m_add_node_callback;
        osm_vector_tile_impl::way_callback_type m_add_way_callback;
        osm_vector_tile_impl::relation_callback_type m_add_relation_callback;

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
         * \brief Parse the response of the database after querying ways
         *
         * Does not clean up memory afterwards!
         *
         * \param result query result
         * \param id OSM object ID to use to construct the objects. Use 0 if it was a spatial query.
         */
        void parse_way_query_result(PGresult* result, osmium::object_id_type id);

        /**
         * \brief Parse the response of the database after querying relations
         *
         * Does not clean up memory afterwards!
         *
         * \param result query result
         * \param id OSM object ID to use to construct the objects. Use 0 if it was a spatial query.
         */
        void parse_relation_query_result(PGresult* result, osmium::object_id_type id);

    public:
        CerepsoDataAccess(VectortileGeneratorConfig& config);

        CerepsoDataAccess(CerepsoDataAccess&& other);

        void set_bbox(const BoundingBox& bbox);

        void set_add_node_callback(osm_vector_tile_impl::node_callback_type&& callback,
                osm_vector_tile_impl::simple_node_callback_type&& simple_callback);

        void set_add_way_callback(osm_vector_tile_impl::way_callback_type&& callback);

        void set_add_relation_callback(osm_vector_tile_impl::relation_callback_type&& callback);

        /**
         * \brief Get all missing relations
         *
         * \param missing_relations relations to fetch from the database
         */
        void get_missing_relations(const osm_vector_tile_impl::osm_id_set_type& missing_relations);

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
        void get_missing_ways(const osm_vector_tile_impl::osm_id_set_type& missing_ways);

        /**
         * \brief Get all relations inside the tile
         */
        void get_relations_inside();

    };

} // namespace input

#endif /* SRC_CEREPSO_DATA_ACCESS_HPP_ */
