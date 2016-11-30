/*
 * util.hpp
 *
 *  Created on: 2016-11-21
 *      Author: Michael Reichert
 */

#ifndef TEST_INCLUDE_UTIL_HPP_
#define TEST_INCLUDE_UTIL_HPP_

#include <iostream>

namespace test_utils {
    /**
     * \both Compare two vectors.
     *
     * Elements must have the same order.
     *
     * Class T must define operator!=(const T, const T).
     *
     * \param vector1 first vector
     * \param vector2 second vector
     */
    template <class T>
    bool compare_vectors(std::vector<T>& vector1, std::vector<T>& vector2) {
        if (vector1.size() != vector2.size()) {
            return false;
        }
        for (unsigned int i = 0; i < vector1.size(); i++) {
            if (vector1.at(i) != vector2.at(i)) {
                return false;
            }
        }
        return true;
    }

    /**
     * \brief Print the contents of a vector.
     *
     * Type T must implement operator<<
     *
     * \param vector reference to vector to be printed
     */
    template <class T>
    void print_vector(std::vector<T>& vector) {
        std::cerr << "{\"";
        for (T& v : vector) {
            std::cerr << v << "\",\"";
        }
        std::cerr << "}\n";
    }

    /**
     * \brief Check if a number is in a given interval (bounds belong to the interval)
     *
     * \param to_check variable to be checked
     * \pararm min lower bound
     * \param max upper bound
     *
     * \return true if to_check is between the bounds
     */
    template <class T>
    bool in_interval(T to_check, T min, T max) {
        if (to_check >= min && to_check <= max) {
            return true;
        }
        return false;
    }
} // namespace test_utils


#endif /* TEST_INCLUDE_UTIL_HPP_ */
