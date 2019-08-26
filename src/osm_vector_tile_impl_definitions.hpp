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

namespace osm_vector_tile_impl {
    using osm_id_set_type = std::set<osmium::object_id_type>;

    using node_callback_type = std::function<void(const osmium::object_id_type, osmium::Location&, const char*, const char*,
            const char*, const char*, const std::string)>;
    using way_callback_type = std::function<void(const osmium::object_id_type, const std::string, const char*, const char*,
            const char*, const char*, const std::string)>;
    using relation_callback_type = std::function<void(const osmium::object_id_type, const std::string,
            const std::string, const std::string, const char*, const char*, const char*,
            const char*, const std::string)>;
}



#endif /* SRC_OSM_VECTOR_TILE_IMPL_DEFINITIONS_HPP_ */
