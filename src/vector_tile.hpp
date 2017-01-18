/*
 * vector_tile.hpp
 *
 *  Created on: 13.10.2016
 *      Author: michael
 */

#ifndef SRC_VECTOR_TILE_HPP_
#define SRC_VECTOR_TILE_HPP_

#include <set>
#include "vectortile_generator_config.hpp"
#include "jobs_database.hpp"

/**
 * \brief class representing a vector tile and providing the public interface to build a vectortile
 *
 * \tparam Vector tile implementation to use which builds the vector tile. This implementation should provide following
 * two public methods (there are no other public methods called):
 * * void TVectorTileImpl::clear(BoundingBox& bbox)
 * * void TVectorTileImpl::generate_vectortile(std::string& output_path)
 *
 * The implementation cares for everything which is related to the output format: querying the database (because some output
 * formats have a special area type, building the entities and writing the file).
 */
template <class TVectorTileImpl>
class VectorTile {
private:
    /// reference to program configuration
    VectortileGeneratorConfig& m_config;

    /// reference to the implementation of the output file
    TVectorTileImpl& m_implementation;

    /// bounding box of this tile
    BoundingBox m_bbox;

    /// pointer to the jobs' database
    JobsDatabase* m_jobs_db;

public:
    /**
     * \brief Constructor to be used if vectortile-generator should only generated all tiles listed in a file (expire tiles format)
     *
     * \param config reference to program configuration, coordinates of the corners of the tile are read from there
     * \param implementation output format implementation to be used
     * \param bbox bounding box of the tile
     * \param jobs_db reference to instance of JobsDatabase
     */
    VectorTile(VectortileGeneratorConfig& config, TVectorTileImpl& implementation, BoundingBox& bbox, JobsDatabase* jobs_db) :
        m_config(config),
        m_implementation(implementation),
        m_bbox(bbox),
        m_jobs_db(jobs_db) {
        m_implementation.clear(bbox);
    }

    /**
     * \brief build the vector tile by calling the method of the choosen implementation and update the jobs database
     */
    void generate_vectortile() {
        // build path where to write the file
        std::string output_path = m_config.m_output_path;
        if (m_config.m_batch_mode) {
            output_path += std::to_string(m_bbox.m_zoom);
            output_path.push_back('_');
            output_path += std::to_string(m_bbox.m_x);
            output_path.push_back('_');
            output_path += std::to_string(m_bbox.m_y);
            output_path.push_back('.');
            output_path += m_config.m_file_suffix;
        }

        // get current time
        time_t rawtime;
        struct tm * ptm;
        time (&rawtime);
        ptm = gmtime(&rawtime);
        char created[21];
        sprintf(created, "%4d-%2d-%2dT%2d:%2d:%2dZ", ptm->tm_year, ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

        // drop previous job if it has not been completed yet
        if (m_jobs_db) { // If the user does not want to write jobs, the unique_ptr doesn't manage anything.
            m_jobs_db->cancel_job(m_bbox.m_x, m_bbox.m_y, m_bbox.m_zoom);
            // check if file exists
            struct stat stat_result;
            if (stat(output_path.c_str(), &stat_result) == 0) {
                // delete old vector tile
                if (std::remove(output_path.c_str())) {
                    std::cerr << "Failed to delete old vector tile " << output_path << '\n';
                }
            }
        }

        m_implementation.generate_vectortile(output_path);

        // insert into jobs database
        if (m_jobs_db) { // If the user does not want to write jobs, the unique_ptr doesn't manage anything.
            m_jobs_db->add_job(m_bbox.m_x, m_bbox.m_y, m_bbox.m_zoom, created, output_path.c_str());
        }
    }
};



#endif /* SRC_VECTOR_TILE_HPP_ */
