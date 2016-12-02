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

/**
 * \brief print program usage and terminate the program
 *
 * \param argv argument values (agrv argument of the main function)
 */
void print_usage(char* argv[]) {
    std::cerr << "Usage: " << argv[0] << " [OPTIONS] [X] [Y] [Z] [OUTFILE]\n" \
                 "or     " << argv[0] << " [OPTIONS] [LOGFILE] [FORMAT] [OUTDIR]\n" \
    "  [X]         x index of a tile\n" \
    "  [Y]         y index of a tile\n" \
    "  [Z]         zoom level of a tile\n" \
    "  [OUTFILE]   output file" \
    "  [LOGFILE]   file containing a list of expired tiles\n" \
    "  [FORMAT]    output format: 'osm', 'osm.pbf', 'opl'\n" \
    "  [OUTDIR]    output directory\n" \
    "  -h, --help                    print help and exit\n" \
    "  -v, --verbose                 be verbose\n" \
    "  -d NAME, --database-name=NAME name of the database where the OSM data is stored\n" \
    "  -j NAME, --jobs-database=NAME name of the database where to write processing jobs\n" \
    "  -r, --recurse-relations       write relations to the output file which are\n" \
    "                                referenced by other relations\n" \
    "  -w, --recurse-ways            write ways to the output file which are beyond\n" \
    "                                the bounding box of the tile and referenced\n" \
    "                                by a relation\n" \
    "  -n, --recurse-nodes           write nodes to the output file which are beyond\n" \
    "                                the bounding box of the tile and referenced\n" \
    "                                by a relation\n" \
    "  -f, --force-overwrite         overwrite output file if it exists\n" \
    "The output format is detected automatically based on the suffix of the output file."<< std::endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
            {"database-name",  required_argument, 0, 'd'},
            {"jobs-database",  required_argument, 0, 'j'},
            {"recurse-relations",  no_argument, 0, 'r'},
            {"recurse-ways",  no_argument, 0, 'w'},
            {"recurse-nodes",  no_argument, 0, 'n'},
            {"force-overwrite",  no_argument, 0, 'f'},
            {"help",  no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };

    VectortileGeneratorConfig config;
    // database related configuration is stored in a separate struct because it is defined by our Postgres access library
    postgres_drivers::Config pg_driver_config;
    while (true) {
        int c = getopt_long(argc, argv, "d:rwnhfvj:", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'd':
                pg_driver_config.m_database_name = optarg;
                break;
            case 'j':
                config.m_jobs_database = optarg;
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
            case 'f':
                config.m_force = true;
                break;
            case 'v':
                config.m_verbose = true;
                break;
            case 'h':
                print_usage(argv);
                break;
            default:
                print_usage(argv);
        }
    }

    int remaining_args = argc - optind;
    std::vector<BoundingBox> bboxes;
    if (remaining_args == 4) {
        config.m_x = atoi(argv[optind]);
        config.m_y = atoi(argv[optind+1]);
        config.m_zoom = atoi(argv[optind+2]);
        config.m_output_path =  argv[optind+3];
        bboxes.push_back(BoundingBox(config));
    } else if (remaining_args == 3) {
        config.m_batch_mode = true;
        bboxes = BoundingBox::read_tiles_list(argv[optind]);
        config.m_file_suffix =  argv[optind+1];
        config.m_output_path =  argv[optind+2];
        // check if last character of the output path is a slash
        if (config.m_output_path.back() != '/') {
            config.m_output_path.push_back('/');
        }
    } else {
        print_usage(argv);
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

    // initialize connection to jobs' database
    JobsDatabase jobs_db(config.m_jobs_database);

    for (BoundingBox& bbox : bboxes) {
        if (config.m_verbose) {
            std::cout << "Creating tile " << bbox.m_zoom << '/' << bbox.m_x << '/' << bbox.m_y << '\n';
        }
        VectorTile vector_tile (config, bbox, nodes_table, untagged_nodes_table, ways_linear_table, relations_table, jobs_db);
        vector_tile.generate_vectortile();
    }
}


