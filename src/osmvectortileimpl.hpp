/*
 * osmvectortileimpl.hpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */

#ifndef SRC_OSMVECTORTILEIMPL_HPP_
#define SRC_OSMVECTORTILEIMPL_HPP_

#include <vector>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/geom/coordinates.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/io/output_iterator.hpp>
#include <osmium/io/writer.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/object_pointer_collection.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/object_comparisons.hpp>
#include <osmium/visitor.hpp>
#include <hstore_parser.hpp>
#include <array_parser.hpp>
#include "bounding_box.hpp"
#include "osm_vector_tile_impl_definitions.hpp"
#include "vectortile_generator_config.hpp"
#include "item_type_conversion.hpp"

using index_type = osmium::index::map::SparseMmapArray<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

/**
 * \brief This class queries the database and builds the OSM entities which it will write to the file.
 *
 * It is one implementation which can be used by VectorTile class. Other implementations can build other output formats.
 *
 * \targ TDataAccess provides access to data required to build the vector tiles
 */
template <class TDataAccess>
class OSMVectorTileImpl {
private:
    /// reference to the program configuration
    VectortileGeneratorConfig& m_config;

    /// provides access to database meeting the needs of this implementation
    TDataAccess& m_data_access;

    /// default buffer size of Osmium's buffer
    static const size_t BUFFER_SIZE = 10240;

    /// buffer where all built OSM objects will reside
    osmium::memory::Buffer m_buffer;

    /// necessary for m_location_handler
    index_type m_index;
    /// nodes we have already fetched from the database
    location_handler_type m_location_handler;

    /// ways we have already fetched from the database
    osm_vector_tile_impl::osm_id_set_type m_ways_got;
    /// relations we have already fetched from the database
    osm_vector_tile_impl::osm_id_set_type m_relations_got;

    /// list of nodes not retrieved by a spatial query but which are necessary to build the ways
    osm_vector_tile_impl::osm_id_set_type m_missing_nodes;
    /// list of ways not retrieved by a spatial query but which are necessary to complete some relations
    osm_vector_tile_impl::osm_id_set_type m_missing_ways;
    /// list of relations not retrieved by a spatial query but which are referenced by other relations
    osm_vector_tile_impl::osm_id_set_type m_missing_relations;

    /**
     * add a tags to an OSM object
     */
    void add_tags(osmium::builder::Builder* builder, const std::string& hstore_content) {
        osmium::builder::TagListBuilder tl_builder(m_buffer, builder);
        pg_array_hstore_parser::HStoreParser hstore (hstore_content);
        while (hstore.has_next()) {
            pg_array_hstore_parser::StringPair kv_pair = hstore.get_next();
            tl_builder.add_tag(kv_pair.first, kv_pair.second);
        }
    }

    /**
     * \brief add node references to a way
     *
     * \param way_builder pointer to the WayBuilder which builds the way
     * \param nodes_array reference to the string which contains the string representation of the array we received from
     * the database
     */
    void add_node_refs(osmium::builder::WayBuilder* way_builder, const std::vector<postgres_drivers::MemberIdPos>& nodes_array) {
        osmium::builder::WayNodeListBuilder wnl_builder{m_buffer, way_builder};
        for (auto& n : nodes_array) {
            check_node_availability(n.id);
            wnl_builder.add_node_ref(osmium::NodeRef(n.id, osmium::Location()));
        }
    }

    /**
     * \brief add members to a relation
     *
     * \param relation_builder pointer to the RelationBuilder which builds the relation
     * \param member_types reference to the string which contains the string representation of the array from member_types column
     * \param member_ids reference to the string which contains the string representation of the array from member_ids column
     * \param member_roles reference to the string which contains the string representation of the array from member_roles column
     */
    void add_relation_members(osmium::builder::RelationBuilder* relation_builder,
            const std::vector<osm_vector_tile_impl::MemberIdRoleTypePos>& members) {
        osmium::builder::RelationMemberListBuilder rml_builder(m_buffer, relation_builder);
        for (auto& m : members) {
            rml_builder.add_member(m.type, m.id, m.role.c_str());
            if (m.type == osmium::item_type::node && m_config.m_recurse_nodes) {
                check_node_availability(m.id);
            } else if (m.type == osmium::item_type::way && m_config.m_recurse_ways) {
                std::set<osmium::object_id_type>::iterator ways_it = m_ways_got.find(m.id);
                if (ways_it == m_ways_got.end()) {
                    m_missing_ways.insert(m.id);
                }
            } else if (m.type == osmium::item_type::relation && m_config.m_recurse_relations) {
                std::set<osmium::object_id_type>::iterator relations_it = m_relations_got.find(m.id);
                if (relations_it == m_relations_got.end()) {
                    m_missing_relations.insert(m.id);
                }
            }
        }
    }

    /**
     * \brief check if a node is available in the location handler and insert it into the list of missing nodes if it is not
     * available in the location handler
     *
     * \param id ID of the node
     */
    void check_node_availability(const osmium::object_id_type id) {
        try {
            osmium::Location location = m_location_handler.get_node_location(id);
            if (!location.valid()) {
                m_missing_nodes.insert(id);
                return;
            }
        } catch (osmium::not_found& e) {
            // This exception is thrown if the node could not be found in the location handler.
            m_missing_nodes.insert(id);
        }
    }

    /**
     * \brief Write vectortile to file and insert a job into the jobs database
     *
     * You should call the destructor after calling this method (e.g. let this instance go out of scope if it is no pointer).
     *
     * \param path path where to write the file
     */
    void write_file(std::string& path) {
        osmium::io::Header header;
        header.set("generator", "vectortile-generator");
        header.set("copyright", "OpenStreetMap and contributors");
        header.set("attribution", "http://www.openstreetmap.org/copyright");
        header.set("license", "http://opendatacommons.org/licenses/odbl/1-0/");
        osmium::io::File output_file{path};
        osmium::io::overwrite overwrite = osmium::io::overwrite::no;
        if (m_config.m_force) {
            overwrite = osmium::io::overwrite::allow;
        }
        osmium::io::Writer writer{output_file, header, overwrite};
        // We have to merge the buffers and sort the objects. Therefore first all nodes are written, then all ways and as last step
        // all relations.
        sort_buffer_and_write_it(writer);
        writer.close();
    }

    /**
     * \brief Add a way to the buffer.
     *
     * This method is intended to be called by the data source implementation.
     *
     * \param id OSM object ID
     * \param nodes nodes as an PostgreSQL array of integers
     * \param version OSM object version
     * \param changeset OSM object changeset attribute
     * \param uid OSM object UID attribute
     * \param timestamp OSM object timestamp
     * \param tags tags to add
     */
    void add_relation(const osmium::object_id_type id,
            const std::vector<osm_vector_tile_impl::MemberIdRoleTypePos> members,
            const char* version, const char* changeset, const char* uid, const char* timestamp,
            const std::string tags) {
        {
            osmium::builder::RelationBuilder relation_builder(m_buffer);
            osmium::Relation& relation = static_cast<osmium::Relation&>(relation_builder.object());
            relation.set_id(id);
            relation.set_changeset(changeset);
            relation.set_uid(uid);
            relation.set_version(version);
            relation.set_visible(true);
            relation.set_timestamp(timestamp);
            relation_builder.set_user("");
            add_tags(&relation_builder, tags);
            add_relation_members(&relation_builder, members);
        }
        m_buffer.commit();
    }

    /**
     * \brief Add a way to the buffer.
     *
     * This method is intended to be called by the data source implementation.
     *
     * \param id OSM object ID
     * \param nodes node references
     * \param version OSM object version
     * \param changeset OSM object changeset attribute
     * \param uid OSM object UID attribute
     * \param timestamp OSM object timestamp
     * \param tags tags to add
     */
    void add_way(const osmium::object_id_type id, const std::vector<postgres_drivers::MemberIdPos> nodes, const char* version,
            const char* changeset, const char* uid, const char* timestamp, const std::string tags) {
        {
            osmium::builder::WayBuilder way_builder(m_buffer);
            osmium::Way& way = static_cast<osmium::Way&>(way_builder.object());
            way.set_id(id);
            way.set_changeset(changeset);
            way.set_uid(uid);
            way.set_version(version);
            way.set_visible(true);
            way.set_timestamp(timestamp);
            way_builder.set_user("");
            add_tags(&way_builder, tags);
            add_node_refs(&way_builder, nodes);
        }
        m_buffer.commit();
    }

    /**
     * \brief Add node to the buffer.
     *
     * \param id OSM object ID
     * \param location location of the node
     * \param version OSM object version
     * \param changeset OSM object changeset attribute
     * \param uid OSM object UID attribute
     * \param timestamp OSM object timestamp
     * \param tags tags to add
     */
    void add_node(const osmium::object_id_type id, const osmium::Location& location,
            const char* version, const char* changeset, const char* uid, const char* timestamp,
            const std::string tags) {

        {
            osmium::builder::NodeBuilder builder(m_buffer);
            osmium::Node& node = static_cast<osmium::Node&>(builder.object());
            node.set_id(id);
            node.set_version(version);
            node.set_changeset(changeset);
            node.set_uid(uid);
            // otherwise the resulting OSM file does not contain the visible=true attribute and some programs behave strange
            node.set_visible(true);
            node.set_timestamp(timestamp);
            builder.set_user("");
            node.set_location(location);
            // Location handler must be called before add_tags(). That's a limitation by Osmium.
            m_location_handler.node(node); // add to location handler to store the location
            if (!tags.empty()) {
                add_tags(&builder, tags);
            }
        }
        m_buffer.commit();
}

    /**
     * \brief sort objects in a buffer and write them to a file
     *
     * Code is taken and modified from [Osmium -- OpenStreetMap data manipulation command line tool](http://osmcode.org/osmium)
     * which is Copyright (C) 2013-2016  Jochen Topf <jochen@topf.org> and available under the terms of
     * GNU General Public License version 3 or newer.
     *
     * \param writer writer which should write the objects to a file
     */
    void sort_buffer_and_write_it(osmium::io::Writer& writer) {
        auto out = osmium::io::make_output_iterator(writer);
        osmium::ObjectPointerCollection objects;
        osmium::apply(m_buffer, objects);
        objects.sort(osmium::object_order_type_id_reverse_version());
        // std::copy (i.e. copy without comparing the objects) does not work. Nodes with tags beyond the
        // bounding box will not be written to the output file.
        std::unique_copy(objects.cbegin(), objects.cend(), out, osmium::object_equal_type_id());
    }

public:
    OSMVectorTileImpl<TDataAccess>(VectortileGeneratorConfig& config, TDataAccess& data_access) :
            m_config(config),
            m_data_access(data_access),
            m_buffer(BUFFER_SIZE, osmium::memory::Buffer::auto_grow::yes),
            m_location_handler(m_index) {

        m_data_access.set_add_node_callback([this](const osmium::object_id_type id,
            const osmium::Location& location, const char* version, const char* changeset,
            const char* uid, const char* timestamp, const std::string tags) {
                this->add_node(id, location, version, changeset, uid, timestamp, tags);
                this->m_buffer.commit();
            }
        );
        m_data_access.set_add_way_callback([this](const osmium::object_id_type id,
            const std::vector<postgres_drivers::MemberIdPos> nodes, const char* version, const char* changeset, const char* uid,
            const char* timestamp, const std::string tags) {
                this->add_way(id, std::move(nodes), version, changeset, uid, timestamp, tags);
                this->m_buffer.commit();
            }
        );
        m_data_access.set_add_relation_callback([this](const osmium::object_id_type id,
            const std::vector<osm_vector_tile_impl::MemberIdRoleTypePos> members,
            const char* version, const char* changeset, const char* uid, const char* timestamp,
            const std::string tags) {
                this->add_relation(id, std::move(members), version, changeset, uid, timestamp,
                        tags);
                this->m_buffer.commit();
            }
        );
    };

    /**
     * \brief Reset the Osmium buffer and all sets (using the clear() method).
     * Change the bounding box of all table members of this class.
     *
     * \param bbox new bounding box
     */
    void clear(BoundingBox& bbox) {
        m_buffer.clear();
        m_location_handler.clear();
        m_ways_got.clear();
        m_relations_got.clear();
        m_missing_nodes.clear();
        m_missing_ways.clear();
        m_missing_relations.clear();
        m_data_access.set_bbox(bbox);
    }

    /**
     * \brief build the vector tile
     *
     * This method will be called by the class VectorTile to start the work.
     *
     * \param output_path location where to write the tile
     */
    void generate_vectortile(std::string& output_path) {
        m_data_access.get_nodes_inside();
        m_data_access.get_ways_inside();
        m_data_access.get_relations_inside();
        if (m_config.m_recurse_relations) {
            m_data_access.get_missing_relations(m_missing_relations);
        }
        if (m_config.m_recurse_ways) {
            m_data_access.get_missing_ways(m_missing_ways);
        }
        m_data_access.get_missing_nodes(m_missing_nodes);
        write_file(output_path);
    }
};


#endif /* SRC_OSMVECTORTILEIMPL_HPP_ */
