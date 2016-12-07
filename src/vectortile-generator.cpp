/*
 * vectortile-generator.cpp
 *
 *  Created on: 11.10.2016
 *      Author: michael
 */


#include <iostream>
#include <getopt.h>
#include <string>
#include <memory>
#include <columns.hpp>
#include "osmvectortileimpl.hpp"
#include "vectortile_generator_config.hpp"
#include "vector_tile.hpp"
#include "osm_data_table.hpp"

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
    "                                This argument is mandatory if you want to write jobs.\n" \
    "  -O, --orphaned-nodes          do a spatial query on the untagged nodes table\n" \
    "                                You really need a spatial index on that table, otherwise\n" \
    "                                you will do a sequential scan on that table! Using -O\n" \
    "                                will return you orphaned nodes you would not have got.\n" \
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
        int c = getopt_long(argc, argv, "d:rwnhfvj:O", long_options, 0);
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
            case 'O':
                config.m_orphaned_nodes = true;
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
    OSMDataTable nodes_table ("nodes", pg_driver_config, node_columns);
    OSMDataTable untagged_nodes_table ("untagged_nodes", pg_driver_config, node_columns);
    OSMDataTable ways_linear_table ("ways", pg_driver_config, node_columns);
    OSMDataTable relations_table ("relations", pg_driver_config, node_columns);

    // initialize the implmenation used to produce the vector tile
    OSMVectorTileImpl vector_tile_impl (config, untagged_nodes_table, nodes_table, ways_linear_table, relations_table);

    // initialize connection to jobs' database
    std::unique_ptr<JobsDatabase> jobs_db;
    if (config.m_jobs_database != "") {
        jobs_db = std::unique_ptr<JobsDatabase>(new JobsDatabase(config.m_jobs_database));
    }

    for (BoundingBox& bbox : bboxes) {
        if (config.m_verbose) {
            std::cout << "Creating tile " << bbox.m_zoom << '/' << bbox.m_x << '/' << bbox.m_y << '\n';
        }
        VectorTile<OSMVectorTileImpl> vector_tile(config, vector_tile_impl, bbox, jobs_db.get());
        vector_tile.generate_vectortile();
    }
}


