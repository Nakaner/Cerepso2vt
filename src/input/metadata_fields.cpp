/*
 * metadata_fields.cpp
 *
 *  Created on:  2019-10-14
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "metadata_fields.hpp"

input::MetadataFields::MetadataFields(VectortileGeneratorConfig& config) :
    m_metadata_field_count(0),
    m_config(config) {
    if (config.m_postgres_config.metadata.user()) {
        m_user_index = m_metadata_field_count;
        ++m_metadata_field_count;
    }
    if (config.m_postgres_config.metadata.uid()) {
        m_uid_index = m_metadata_field_count;
        ++m_metadata_field_count;
    }
    if (config.m_postgres_config.metadata.version()) {
        m_version_index = m_metadata_field_count;
        ++m_metadata_field_count;
    }
    if (config.m_postgres_config.metadata.timestamp()) {
        m_last_modified_index = m_metadata_field_count;
        ++m_metadata_field_count;
    }
    if (config.m_postgres_config.metadata.changeset()) {
        m_changeset_index = m_metadata_field_count;
        ++m_metadata_field_count;
    }
}

int input::MetadataFields::count() {
    return m_metadata_field_count;
}

int input::MetadataFields::user() {
    return m_user_index;
}

int input::MetadataFields::uid() {
    return m_uid_index;
}

int input::MetadataFields::version() {
    return m_version_index;
}

int input::MetadataFields::last_modified() {
    return m_last_modified_index;
}

bool input::MetadataFields::changeset() {
    return m_changeset_index;
}

bool input::MetadataFields::has_user() {
    return m_user_index != unavailable;
}

bool input::MetadataFields::has_uid() {
    return m_uid_index != unavailable;
}

bool input::MetadataFields::has_version() {
    return m_version_index != unavailable;
}

bool input::MetadataFields::has_last_modified() {
    return m_last_modified_index != unavailable;
}

bool input::MetadataFields::has_changeset() {
    return m_changeset_index != unavailable;
}

/**
 * Get the beginning of an SQL SELECT string containing all requested metadata fields.
 */
std::string input::MetadataFields::select_str() const {
    std::string query = "SELECT ";
    if (m_config.m_postgres_config.metadata.user()) {
        query.append("osm_user, ");
    }
    if (m_config.m_postgres_config.metadata.uid()) {
        query.append("osm_uid, ");
    }
    if (m_config.m_postgres_config.metadata.version()) {
        query.append("osm_version, ");
    }
    if (m_config.m_postgres_config.metadata.timestamp()) {
        query.append("osm_lastmodified, ");
    }
    if (m_config.m_postgres_config.metadata.changeset()) {
        query.append("osm_changeset, ");
    }
    return query;
}
