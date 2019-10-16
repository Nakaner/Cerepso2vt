/*
 * untagged_nodes_provider_factory.hpp
 *
 *  Created on:  2019-10-10
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_INPUT_CEREPSO_UNTAGGED_NODES_PROVIDER_FACTORY_HPP_
#define SRC_INPUT_CEREPSO_UNTAGGED_NODES_PROVIDER_FACTORY_HPP_

#include "nodes_provider.hpp"

namespace input {

    namespace cerepso {

        class NodesProviderFactory {
        public:
            /**
             * Create a provider for untagged nodes from a database table.
             */
            static std::unique_ptr<NodesProvider> db_provider(
                    VectortileGeneratorConfig& config, OSMDataTable&& nodes_table, OSMDataTable&& untagged_nodes_table);
            /**
             * Create a location handler.
             */
            static std::unique_ptr<NodesProvider> flatnodes_provider(
                    VectortileGeneratorConfig& config, OSMDataTable&& nodes_table);
        };

    } // namespace cerepso

} // namespace input


#endif /* SRC_INPUT_CEREPSO_UNTAGGED_NODES_PROVIDER_FACTORY_HPP_ */
