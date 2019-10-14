/*
 * metadata_fields.hpp
 *
 *  Created on:  2019-10-10
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_INPUT_CEREPSO_METADATA_FIELDS_HPP_
#define SRC_INPUT_CEREPSO_METADATA_FIELDS_HPP_

#include <string>
#include "../../vectortile_generator_config.hpp"

namespace input {

    namespace cerepso {

        class MetadataFields {
        public:
            static constexpr int unavailable = -1;

        private:
            int m_metadata_field_count;
            VectortileGeneratorConfig& m_config;
            int m_user_index = unavailable;
            int m_uid_index = unavailable;
            int m_version_index = unavailable;
            int m_last_modified_index = unavailable;
            int m_changeset_index = unavailable;

        public:
            MetadataFields(VectortileGeneratorConfig& config);

            int count();

            int user();

            int uid();

            int version();

            int last_modified();

            bool changeset();

            bool has_user();

            bool has_uid();

            bool has_version();

            bool has_last_modified();

            bool has_changeset();

            /**
             * Get the beginning of an SQL SELECT string containing all requested metadata fields.
             */
            std::string select_str() const;
        };
    }
}



#endif /* SRC_INPUT_CEREPSO_METADATA_FIELDS_HPP_ */
