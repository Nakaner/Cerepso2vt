/*
 * vector_tile.cpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */
#include <osmium/io/any_output.hpp>
#include <osmium/io/file.hpp>
#include <osmium/io/output_iterator.hpp>
#include <osmium/object_pointer_collection.hpp>
#include <osmium/osm/object_comparisons.hpp>
#include "vector_tile.hpp"

VectorTile::VectorTile(VectortileGeneratorConfig& config, MyTable& untagged_nodes_table, MyTable& nodes_table, MyTable& ways_table,
        MyTable& relations_table) :
        m_config(config),
        m_untagged_nodes_table(untagged_nodes_table),
        m_nodes_table(nodes_table),
        m_ways_table(ways_table),
        m_relations_table(relations_table),
        m_buffer(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes),
        m_location_handler(m_index),
        m_bbox(config.m_x, config.m_y, config.m_zoom) {
    std::cout << m_bbox.m_min_lon << " " << m_bbox.m_min_lat << " " << m_bbox.m_max_lon << " " << m_bbox.m_max_lat << "\n";
}

VectorTile::VectorTile(VectortileGeneratorConfig& config, BoundingBox& bbox, MyTable& untagged_nodes_table, MyTable& nodes_table, MyTable& ways_table,
        MyTable& relations_table) :
        m_config(config),
        m_untagged_nodes_table(untagged_nodes_table),
        m_nodes_table(nodes_table),
        m_ways_table(ways_table),
        m_relations_table(relations_table),
        m_buffer(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes),
        m_location_handler(m_index),
        m_bbox(bbox){ }

void VectorTile::get_nodes_inside() {
    m_nodes_table.get_nodes_inside(m_buffer, m_location_handler, m_bbox);
    m_untagged_nodes_table.get_nodes_inside(m_buffer, m_location_handler, m_bbox);
}

void VectorTile::get_missing_nodes() {
    m_nodes_table.get_missing_nodes(m_buffer, m_missing_nodes);
    m_untagged_nodes_table.get_missing_nodes(m_buffer, m_missing_nodes);
}

void VectorTile::get_missing_ways() {
    m_ways_table.get_missing_ways(m_buffer, m_missing_ways, m_location_handler, m_missing_nodes);
}

void VectorTile::get_ways_inside() {
    m_ways_table.get_ways_inside(m_buffer, m_bbox, m_location_handler, m_missing_nodes, m_ways_got);
}

void VectorTile::get_relations_inside() {
    m_relations_table.get_relations_inside(m_buffer, m_bbox, m_location_handler, m_missing_nodes, m_missing_ways,
            m_missing_relations, m_ways_got, m_relations_got);
}

void VectorTile::get_missing_relations() {
    if (m_config.m_recurse_nodes) {
        m_relations_table.get_missing_relations(m_buffer, m_missing_relations, m_ways_got, m_missing_ways,
                m_location_handler, &m_missing_nodes);
    } else {
        m_relations_table.get_missing_relations(m_buffer, m_missing_relations, m_ways_got, m_missing_ways,
                m_location_handler, nullptr);
    }
}

void VectorTile::generate_vectortile() {
    get_nodes_inside();
    get_ways_inside();
    get_relations_inside();

    if (m_config.m_recurse_relations) {
        get_missing_relations();
    }
    if (m_config.m_recurse_ways) {
        get_missing_ways();
    }
    get_missing_nodes();
    write_file();
}

/*static*/ void VectorTile::sort_buffer_and_write_it(osmium::memory::Buffer& buffer, osmium::io::Writer& writer) {
    auto out = osmium::io::make_output_iterator(writer);
    osmium::ObjectPointerCollection objects;
    osmium::apply(buffer, objects);
    objects.sort(osmium::object_order_type_id_reverse_version());
    std::unique_copy(objects.cbegin(), objects.cend(), out, osmium::object_equal_type_id());
}

void VectorTile::write_file() {
    osmium::io::Header header;
    header.set("generator", "vectortile-generator");
    header.set("copyright", "OpenStreetMap and contributors");
    header.set("attribution", "http://www.openstreetmap.org/copyright");
    header.set("license", "http://opendatacommons.org/licenses/odbl/1-0/");
    osmium::io::File output_file{m_config.m_output_file};
    osmium::io::overwrite overwrite = osmium::io::overwrite::no;
    if (m_config.m_force) {
        overwrite = osmium::io::overwrite::allow;
    }
    osmium::io::Writer writer{output_file, header, overwrite};
    // We have to merge the buffers and sort the objects. Therefore first all nodes are written, then all ways and as last step
    // all relations.
    sort_buffer_and_write_it(m_buffer, writer);
    writer.close();
}
