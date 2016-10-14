/*
 * vectortile-generator.cpp
 *
 *  Created on: 11.10.2016
 *      Author: michael
 */


#include <iostream>
#include <getopt.h>
#include <string>
#include <osmium/io/any_output.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/io/file.hpp>
#include <osmium/io/header.hpp>
#include <osmium/io/writer.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/builder/osm_object_builder.hpp>


void set_dummy_osm_object_attributes(osmium::OSMObject& obj) {
    obj.set_version("1");
    obj.set_changeset("5");
    obj.set_uid("140");
    obj.set_timestamp("2016-01-05T01:22:45Z");
}

void add_tags(osmium::memory::Buffer& buffer, osmium::builder::Builder& builder) {
    osmium::builder::TagListBuilder tl_builder(buffer, &builder);
    tl_builder.add_tag("highway", "trunk");
}

void add_rel_members(osmium::builder::RelationBuilder& builder, osmium::memory::Buffer& buffer) {
    osmium::builder::RelationMemberListBuilder rml_builder(buffer, &builder);
    rml_builder.add_member(osmium::item_type::way, 1, "forward");
}

int main(int argc, char* argv[]) {

    static struct option long_options[] = {
            {"database",  required_argument, 0, 'd'},
            {0, 0, 0, 0}
        };
    std::string database = "pgimportertest";
    while (true) {
        int c = getopt_long(argc, argv, "d:", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'd':
                database = optarg;
                break;
            default:
                exit(1);
        }
    }

    int remaining_args = argc - optind;
    if (remaining_args != 1) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] [INFILE]\n" \
        "  -a, --append                     this is a diff import\n" \
        "  -d, --database-name              database name\n" \
        "  -e FILE, --expire-tiles=FILE     write an expiry_tile list to FILE\n" \
        "  -E TYPE, --expiry-generator=TYPE choose TYPE as expiry list generator\n" \
        "  -g, --no-geom-indexes            don't create any geometry indexes\n" \
        "  -G, --all-geom-indexes           create geometry indexes on all tables (otherwise not on untagged nodes table),\n" \
        "                                   overrides -g"
        "  -I, --no-id-index                don't create an index on osm_id columns\n" \
        "  --min-zoom=ZOOM                  minimum zoom for expire_tile list\n" \
        "  --max-zoom=ZOOM                  maximum zoom for expire_tile list\n" \
        "  -l, --location-handler=HANDLER   use HANDLER as location handler\n" \
        "  -o, --no-order-by-geohash        don't order tables by ST_GeoHash\n" \
        "  -u, --no-usernames               don't insert user names into the database\n" << std::endl;
        exit(1);
    } else {
        config.m_osm_file =  argv[optind];
    }



//    const int buffer_size = 10240;
//    osmium::memory::Buffer node_buffer(buffer_size, osmium::memory::Buffer::auto_grow::yes);
//    osmium::memory::Buffer way_buffer(buffer_size, osmium::memory::Buffer::auto_grow::yes);
//    osmium::memory::Buffer rel_buffer(buffer_size, osmium::memory::Buffer::auto_grow::yes);
//
//    // node 1
//    osmium::Location location1(9.0, 49.0);
//    osmium::builder::NodeBuilder builder(node_buffer);
//    static_cast<osmium::Node&>(builder.object()).set_id(1);
//    set_dummy_osm_object_attributes(static_cast<osmium::OSMObject&>(builder.object()));
//    builder.add_user("foo");
//    static_cast<osmium::Node&>(builder.object()).set_location(location1);
//    add_tags(node_buffer, builder);
//
//
//    // node 2
//    osmium::Location location2(9.1, 49.1);
//    osmium::builder::NodeBuilder builder2(node_buffer);
//    static_cast<osmium::Node&>(builder2.object()).set_id(2);
//    set_dummy_osm_object_attributes(static_cast<osmium::OSMObject&>(builder2.object()));
//    builder2.add_user("foo");
//    static_cast<osmium::Node&>(builder2.object()).set_location(location2);
//    add_tags(node_buffer, builder2);
//
//
//    // way
//    osmium::builder::WayBuilder way_builder(way_buffer);
//    static_cast<osmium::Way&>(way_builder.object()).set_id(1);
//    set_dummy_osm_object_attributes(static_cast<osmium::OSMObject&>(way_builder.object()));
//    way_builder.add_user("foo");
//    // way node list
//    osmium::builder::WayNodeListBuilder wnl_builder(way_buffer, &way_builder);
//    osmium::NodeRef node_ref1 (1, location1);
//    osmium::NodeRef node_ref2 (2, location2);
//    wnl_builder.add_node_ref(node_ref1);
//    wnl_builder.add_node_ref(node_ref2);
//    add_tags(way_buffer, way_builder);
//
//
//    // relation
//    osmium::builder::RelationBuilder rel_builder(rel_buffer);
//    static_cast<osmium::Relation&>(rel_builder.object()).set_id(1);
//    set_dummy_osm_object_attributes(static_cast<osmium::OSMObject&>(rel_builder.object()));
//    rel_builder.add_user("foo");
//    add_tags(rel_buffer, rel_builder);
//    add_rel_members(rel_builder, rel_buffer);
//
//
//    node_buffer.commit();
//    way_buffer.commit();
//    rel_buffer.commit();
//
//    osmium::io::File outfile("/tmp/mytest.osm");
//    osmium::io::Header hdr;
//    hdr.set("generator", "osmium");
//    osmium::io::Writer writer(outfile, hdr, osmium::io::overwrite::allow);
//    writer(std::move(node_buffer));
//    writer(std::move(way_buffer));
//    writer(std::move(rel_buffer));
//    writer.close();
}


