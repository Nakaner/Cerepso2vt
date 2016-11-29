/*
 * vectortile-generator.cpp
 *
 *  Created on: 11.10.2016
 *      Author: michael
 */


#include <iostream>
#include <getopt.h>
#include <string>
#include <columns.hpp>
#include "vectortile_generator_config.hpp"
#include "mytable.hpp"
#include "vector_tile.hpp"

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
            {"database",  required_argument, 0, 'd'},
            {0, 0, 0, 0}
        };

    VectortileGeneratorConfig config;
    // database related configuration is stored in a separate struct because it is defined by our Postgres access library
    postgres_drivers::Config pg_driver_config;
    while (true) {
        int c = getopt_long(argc, argv, "d:rwn", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'd':
                pg_driver_config.m_database_name = optarg;
                break;
            case 'r':
                config.m_recurse_relations = true;
                break;
            case 'w':
                config.m_recurse_ways = true;
                break;
            case 'n':
                config.m_recurse_nodes = true;
                break;
            default:
                exit(1);
        }
    }

    int remaining_args = argc - optind;
    if (remaining_args != 4) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [X] [Y] [Z] [OUTFILE]\n" \
        "  -d, --database-name              database name\n" \
        "  -r, --recurse-relations          write relations to the output file which are\n" \
        "                                   referenced by other relations\n" \
        "  -w, --recurse-ways               write ways to the output file which are beyond\n" \
        "                                   the bounding box of the tile and referenced\n" \
        "                                   by a relation\n" \
        "  -n, --recurse-nodes              write nodes to the output file which are beyond\n" \
        "                                   the bounding box of the tile and referenced\n" \
        "                                   by a relation\n" \
        "The output format is detected automatically based on the suffix of the output file."<< std::endl;
        exit(1);
    } else {
        config.m_x = atoi(argv[optind]);
        config.m_y = atoi(argv[optind+1]);
        config.m_zoom = atoi(argv[optind+2]);
        config.m_output_file =  argv[optind+3];
    }

    // column definitions
    postgres_drivers::Columns node_columns(pg_driver_config, postgres_drivers::TableType::POINT);
    postgres_drivers::Columns untagged_nodes_columns(pg_driver_config, postgres_drivers::TableType::UNTAGGED_POINT);
    postgres_drivers::Columns way_linear_columns(pg_driver_config, postgres_drivers::TableType::WAYS_LINEAR);
    postgres_drivers::Columns relation_other_columns(pg_driver_config, postgres_drivers::TableType::RELATION_OTHER);

    // intialize connection to database tables
    MyTable nodes_table ("nodes", pg_driver_config, node_columns, config);
    MyTable untagged_nodes_table ("untagged_nodes", pg_driver_config, untagged_nodes_columns, config);
    MyTable ways_linear_table ("ways", pg_driver_config, way_linear_columns, config);
    MyTable relations_table("relations", pg_driver_config, relation_other_columns, config);

    VectorTile vector_tile (config, nodes_table, untagged_nodes_table,
            ways_linear_table, relations_table);
    vector_tile.generate_vectortile();
}


