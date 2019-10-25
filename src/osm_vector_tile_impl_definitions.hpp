/*
 * osm_vectort_tile_impl_definitions.hpp
 *
 *  Created on:  2019-08-20
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_OSM_VECTOR_TILE_IMPL_DEFINITIONS_HPP_
#define SRC_OSM_VECTOR_TILE_IMPL_DEFINITIONS_HPP_

#include <functional>
#include <set>
#include <string>
#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>
#include <postgres_drivers/table.hpp>

namespace osm_vector_tile_impl {
    using osm_id_set_type = std::set<osmium::object_id_type>;

    struct MemberIdRoleTypePos : public postgres_drivers::MemberIdTypePos {
        std::string role;

        MemberIdRoleTypePos(osmium::object_id_type id, osmium::item_type type, std::string role, int pos) :
            MemberIdTypePos(id, type, pos),
            role(role) {
        }
    };

    using StringPair = std::pair<std::string, std::string>;

    enum metadata_fields : char {
        NONE      = 0x00,
        VERSION   = 0x01,
        TIMESTAMP = 0x02,
        CHANGESET = 0x04,
        UID       = 0x08,
        USER      = 0x10
    };

    using node_callback_type = std::function<void(const osmium::object_id_type, osmium::Location&, const char*, const char*,
            const char*, const char*, const std::string, const postgres_drivers::ColumnsVector&,
            const std::vector<const char*>&)>;
    using node_without_tags_callback_type = std::function<void(const osmium::object_id_type, osmium::Location&, const char*, const char*,
            const char*, const char*)>;
    using simple_node_callback_type = std::function<void(const osmium::object_id_type, osmium::Location&)>;
    using way_callback_type = std::function<void(const osmium::object_id_type,
            const std::vector<postgres_drivers::MemberIdPos>, const char*, const char*,
            const char*, const char*, const std::string, const postgres_drivers::ColumnsVector&,
            const std::vector<const char*>&)>;
    using slim_way_callback_type = std::function<void(const osmium::object_id_type,
            const std::vector<postgres_drivers::MemberIdPos>, const char*, const char*,
            const char*, const char*, const std::vector<osm_vector_tile_impl::StringPair>&)>;
    using relation_callback_type = std::function<void(const osmium::object_id_type,
            const std::vector<MemberIdRoleTypePos>, const char*, const char*, const char*,
            const char*, const std::string, const postgres_drivers::ColumnsVector&,
            const std::vector<const char*>&)>;
    using slim_relation_callback_type = std::function<void(const osmium::object_id_type,
            const std::vector<MemberIdRoleTypePos>, const char*, const char*, const char*,
            const char*, const std::vector<osm_vector_tile_impl::StringPair>&)>;
}



#endif /* SRC_OSM_VECTOR_TILE_IMPL_DEFINITIONS_HPP_ */
