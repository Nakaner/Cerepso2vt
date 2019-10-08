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
#include <postgres_drivers/columns.hpp>
#include "cerepso_data_access.hpp"
#include "osmvectortileimpl.hpp"
#include "vectortile_generator_config.hpp"
#include "vector_tile.hpp"
#include "osm_data_table.hpp"

/**
 * \mainpage
 * Cerepso2vt
 * ==========
 *
 * Cerepso2vt is a Osmium-based C++ program to create vector tiles in
 * OpenStreetMap's common data formats – OSM XML and OSM PBF. Cerepso2vt
 * supports all formats Osmium can write and was designed to be flexible.
 * It should be easy to add different export formats.
 * Cerepso2vt's data source is a PostgreSQL database which was imported
 * using Cerepso.
 *
 *
 * Dependencies
 * ------------
 *
 * You have to install following dependencies
 *
 * * libosmium
 * * libproj-dev
 * * libpq (use the packages of your distribution)
 * * C++11 compiler, e.g. g++4.6 or newer
 *
 * All other dependencies are shipped in the contrib/ directory.
 *
 *
 * Building
 * --------
 *
 *     mkdir build
 *     cd build
 *     ccmake ..
 *     make
 *
 *
 * Unit Tests
 * ----------
 *
 * Unit tests are located in the `test/` directory. We use the Catch framework, all
 * necessary depencies are included in this repository.
 *
 *
 * License
 * -------
 * This program is available under the terms of GNU General Public License 2.0 or
 * newer. For the legal code of GNU General Public License 2.0 see the file COPYING.md in this directory.
 */

/**
 * \brief print program usage and terminate the program
 *
 * \param argv argument values (argv argument of the main function)
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
    "  --metadata=OPTARG             OSM output only: import specified metadata fields. Permitted values are \"none\", \"all\" and\n" \
    "                                one or many of the following values concatenated by \"+\": version,\n" \
    "                                timestamp, user, uid, changeset.\n" \
    "                                Valid examples: \"none\" (no metadata), \"all\" (all fields),\n" \
    "                                \"version+timestamp\" (only version and timestamp).\n" \
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
            {"untagged-nodes-geom",  no_argument, 0, 200},
            {"metadata",  required_argument, 0, 201},
            {"help",  no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };

    VectortileGeneratorConfig config;
    while (true) {
        int c = getopt_long(argc, argv, "d:rwnhfvj:O", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'd':
                config.m_postgres_config.m_database_name = optarg;
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
            case 200:
                config.m_untagged_nodes_geom = true;
                break;
            case 201:
                config.m_postgres_config.metadata = osmium::metadata_options(optarg);
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
    postgres_drivers::Columns node_columns(config.m_postgres_config, postgres_drivers::TableType::POINT);
    postgres_drivers::Columns untagged_nodes_columns(config.m_postgres_config, postgres_drivers::TableType::UNTAGGED_POINT);
    postgres_drivers::Columns way_linear_columns(config.m_postgres_config, postgres_drivers::TableType::WAYS_LINEAR);
    postgres_drivers::Columns node_ways_columns(config.m_postgres_config, postgres_drivers::TableType::NODE_WAYS);
    postgres_drivers::Columns relation_other_columns(config.m_postgres_config, postgres_drivers::TableType::RELATION_OTHER);
    postgres_drivers::Columns node_relations_columns(config.m_postgres_config, postgres_drivers::TableType::RELATION_MEMBER_NODES);
    postgres_drivers::Columns way_relations_columns(config.m_postgres_config, postgres_drivers::TableType::RELATION_MEMBER_WAYS);

    // intialize connection to database tables
    OSMDataTable nodes_table ("planet_osm_point", config.m_postgres_config, node_columns);
    OSMDataTable untagged_nodes_table ("untagged_nodes", config.m_postgres_config, untagged_nodes_columns);
    OSMDataTable ways_linear_table ("planet_osm_line", config.m_postgres_config, way_linear_columns);
    OSMDataTable node_ways_table ("node_ways", config.m_postgres_config, node_ways_columns);
    OSMDataTable relations_table ("relations", config.m_postgres_config, relation_other_columns);
    OSMDataTable node_relations_table ("node_relations", config.m_postgres_config, node_relations_columns);
    OSMDataTable way_relations_table ("way_relations", config.m_postgres_config, way_relations_columns);

    // initialize the implmenation used to produce the vector tile
    CerepsoDataAccess data_access {config, untagged_nodes_table, nodes_table, ways_linear_table,
        relations_table, node_ways_table, node_relations_table, way_relations_table};
    OSMVectorTileImpl<CerepsoDataAccess> vector_tile_impl {config, data_access};

    // initialize connection to jobs' database
    std::unique_ptr<JobsDatabase> jobs_db;
    if (config.m_jobs_database != "") {
        jobs_db = std::unique_ptr<JobsDatabase>(new JobsDatabase(config.m_jobs_database));
    }

    for (BoundingBox& bbox : bboxes) {
        if (config.m_verbose) {
            std::cout << "Creating tile " << bbox.m_zoom << '/' << bbox.m_x << '/' << bbox.m_y << '\n';
        }
        VectorTile<OSMVectorTileImpl<CerepsoDataAccess>> vector_tile(config, vector_tile_impl, bbox, jobs_db.get());
        vector_tile.generate_vectortile();
    }
}


