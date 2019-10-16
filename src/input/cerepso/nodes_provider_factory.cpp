/*
 * nodes_provider_factory.cpp
 *
 *  Created on:  2019-10-11
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "nodes_provider_factory.hpp"
#include "nodes_db_provider.hpp"
#include "nodes_flatnode_provider.hpp"

using namespace input::cerepso;

/*static*/ std::unique_ptr<NodesProvider> NodesProviderFactory::db_provider(VectortileGeneratorConfig& config,
        OSMDataTable&& nodes_table, OSMDataTable&& untagged_nodes_table) {
    return std::unique_ptr<NodesProvider>{static_cast<NodesProvider*>(new NodesDBProvider{config,
        std::move(nodes_table), std::move(untagged_nodes_table)})};
}
/**
 * Create a location handler.
 */
/*static*/ std::unique_ptr<NodesProvider> NodesProviderFactory::flatnodes_provider(VectortileGeneratorConfig& config,
        OSMDataTable&& nodes_table) {
    return std::unique_ptr<NodesProvider>{static_cast<NodesProvider*>(new NodesFlatnodeProvider{
        config, std::move(nodes_table)})};
}


