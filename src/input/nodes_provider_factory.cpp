/*
 * nodes_provider_factory.cpp
 *
 *  Created on:  2019-10-11
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "nodes_provider_factory.hpp"
#include "nodes_db_provider.hpp"
#include "nodes_flatnode_provider.hpp"

/*static*/ std::unique_ptr<input::NodesProvider> input::NodesProviderFactory::db_provider(
        VectortileGeneratorConfig& config, ColumnConfigParser& column_config_parser,
        OSMDataTable&& nodes_table, OSMDataTable&& untagged_nodes_table) {
    return std::unique_ptr<NodesProvider>{static_cast<NodesProvider*>(new input::NodesDBProvider{config,
        column_config_parser, std::move(nodes_table), std::move(untagged_nodes_table)})};
}
/**
 * Create a location handler.
 */
/*static*/ std::unique_ptr<input::NodesProvider> input::NodesProviderFactory::flatnodes_provider(
        VectortileGeneratorConfig& config, ColumnConfigParser& column_config_parser,
        OSMDataTable&& nodes_table) {
    return std::unique_ptr<input::NodesProvider>{static_cast<input::NodesProvider*>(new input::NodesFlatnodeProvider{
        config, column_config_parser, std::move(nodes_table)})};
}


