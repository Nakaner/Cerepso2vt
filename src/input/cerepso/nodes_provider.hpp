/*
 * cerepso_untagged_nodes_provider.hpp
 *
 *  Created on:  2019-10-09
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_CEREPSO_UNTAGGED_NODES_PROVIDER_HPP_
#define SRC_CEREPSO_UNTAGGED_NODES_PROVIDER_HPP_

#include <osmium/osm/metadata_options.hpp>
#include "metadata_fields.hpp"
#include "../../bounding_box.hpp"
#include "../../osm_data_table.hpp"
#include "../../osm_vector_tile_impl_definitions.hpp"
#include "../../vectortile_generator_config.hpp"

namespace input {

    namespace cerepso {

        /**
         * Provide access to storage of untagged nodes no matter how it is implemented in detail.
         */
        class NodesProvider {
        protected:
            VectortileGeneratorConfig& m_config;

            /// reference to `nodes` table
            OSMDataTable m_nodes_table;

            MetadataFields m_metadata;

            osm_vector_tile_impl::node_callback_type m_add_node_callback;

            osm_vector_tile_impl::simple_node_callback_type m_add_simple_node_callback;

            /**
             * Get the location of a node from the storage of untagged nodes.
             *
             * \param id node ID
             * \param param_values array whose first and only member is a C-string with the ID of the node
             *
             * \returns true if found
             */
            bool get_node_with_tags(const osmium::object_id_type id, char** param_values);

            void create_prepared_statements();

        public:
            NodesProvider(VectortileGeneratorConfig& config, const char* nodes_table_name);

            virtual ~NodesProvider();

            void set_add_node_callback(osm_vector_tile_impl::node_callback_type& callback);

            void set_add_simple_node_callback(osm_vector_tile_impl::simple_node_callback_type& callback);

            void set_bbox(const BoundingBox& bbox);

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
            virtual void get_missing_nodes(const osm_vector_tile_impl::osm_id_set_type& m_missing_nodes) = 0;

            /**
             * \brief Parse the response of the database after querying nodes
             *
             * Does not clean up memory afterwards!
             *
             * \param result query result
             * \param with_tags True if nodes table was queried
             * \param id OSM object ID to use to construct the objects. Use 0 if it was a spatial query.
             */
            void parse_node_query_result(PGresult* result, const bool with_tags,
                    const osmium::object_id_type id);
        };

    } // namespace cerepso

} // namespace input

#endif /* SRC_CEREPSO_UNTAGGED_NODES_PROVIDER_HPP_ */
