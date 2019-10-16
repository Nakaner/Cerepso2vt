/*
 * untagged_nodes_flatnode_provider.hpp
 *
 *  Created on:  2019-10-10
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_INPUT_CEREPSO_UNTAGGED_NODES_FLATNODE_PROVIDER_HPP_
#define SRC_INPUT_CEREPSO_UNTAGGED_NODES_FLATNODE_PROVIDER_HPP_

#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/dense_file_array.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>
#include "nodes_provider.hpp"

namespace input {

    namespace cerepso {

        using dense_file_array_t = osmium::index::map::DenseFileArray<osmium::unsigned_object_id_type, osmium::Location>;

        class NodesFlatnodeProvider : public NodesProvider {
            // We have to have the unique_ptr as a member to prevent that it goes out of scope.
            dense_file_array_t m_storage_pos;

            osmium::handler::NodeLocationsForWays<dense_file_array_t> m_location_handler;

            dense_file_array_t load_index(const char* path);

        public:
            NodesFlatnodeProvider() = delete;

            NodesFlatnodeProvider(VectortileGeneratorConfig& config, OSMDataTable&& nodes_table);

            ~NodesFlatnodeProvider();

            void get_nodes_inside();

            void get_missing_nodes(const osm_vector_tile_impl::osm_id_set_type& missing_nodes);
        };

    } // namespace cerepso

} // namespace input



#endif /* SRC_INPUT_CEREPSO_UNTAGGED_NODES_FLATNODE_PROVIDER_HPP_ */
