/*
 * type_conversion.hpp
 *
 *  Created on: 2016-11-21
 *      Author: Michael Reichert
 */

#ifndef SRC_TYPE_CONVERSION_HPP_
#define SRC_TYPE_CONVERSION_HPP_

#include <string>

/**
 * \brief TypeConversionImpl implementation to be used with TypeConversion class if the output format should be std::string.
 *
 * This implements the Null Object Pattern.
 */
class StringConversionImpl {
public:
    using output_type = std::string;

    output_type to_output_format(std::string& str) {
        return str;
    }
};

/**
 * \brief TypeConversionImpl implementation to be used with the TypeConversion class if the output format should be int64_t.
 */
class Int64ConversionImpl {
public:
    using output_type = int64_t;

    output_type to_output_format(std::string& str) {
        return std::strtoll(str.c_str(), NULL, 10);
    }
};

/**
 * \brief Template class for type conversions during parsing arrays received from a PostgreSQL query response (arrays are
 * represented as strings).
 *
 * This uses the same pattern as Osmium's osmium::geom::GeometryFactory class.
 * https://github.com/osmcode/libosmium/blob/master/include/osmium/geom/factory.hpp
 */
template <typename TypeConversionImpl>
class TypeConversion {
    TypeConversionImpl m_impl;

public:
    using output_type = typename TypeConversionImpl::output_type;

    /**
     * \brief Convert to the output format.
     *
     * \param str string to be converted
     */
    output_type to_output_format(std::string& str) {
        return m_impl.to_output_format(str);
    }
};

using StringConversion = TypeConversion<StringConversionImpl>;

using Int64Conversion = TypeConversion<Int64ConversionImpl>;


#endif /* SRC_TYPE_CONVERSION_HPP_ */
