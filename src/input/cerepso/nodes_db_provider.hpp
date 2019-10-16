/*
 * untagged_nodes_db_provider.hpp
 *
 *  Created on:  2019-10-09
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_INPUT_CEREPSO_UNTAGGED_NODES_DB_PROVIDER_HPP_
#define SRC_INPUT_CEREPSO_UNTAGGED_NODES_DB_PROVIDER_HPP_

#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>
#include "../../osm_data_table.hpp"
#include "nodes_provider.hpp"

namespace input {

    namespace cerepso {

        class NodesDBProvider : public NodesProvider {
            /// reference to the program configuration
            VectortileGeneratorConfig& m_config;

            OSMDataTable m_untagged_nodes_table;

            void create_prepared_statements_untagged();

        public:
            NodesDBProvider() = delete;

            NodesDBProvider(NodesDBProvider&) = delete;

            NodesDBProvider(VectortileGeneratorConfig& config, OSMDataTable&& nodes_table, OSMDataTable&& untagged_nodes_table);

            ~NodesDBProvider();

            void set_bbox(const BoundingBox& bbox);

            void get_nodes_inside();

            void get_missing_nodes(const osm_vector_tile_impl::osm_id_set_type& missing_nodes);
        };

    } // namespace cerepso

} // namespace input



#endif /* SRC_INPUT_CEREPSO_UNTAGGED_NODES_DB_PROVIDER_HPP_ */
