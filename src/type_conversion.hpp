/*
 * type_conversion.hpp
 *
 *  Created on: 21.11.2016
 *      Author: Michael Reichert
 */

#ifndef SRC_TYPE_CONVERSION_HPP_
#define SRC_TYPE_CONVERSION_HPP_

#include <string>

class StringConversionImpl {
public:
    using output_type = std::string;

    output_type to_output_format(std::string& str);
};

class Int64ConversionImpl {
public:
    using output_type = int64_t;

    output_type to_output_format(std::string& str) {
        return std::strtoll(str.c_str(), NULL, 10);
    }
};


template <typename TypeConversionImpl>
class TypeConversion {
    TypeConversionImpl m_impl;

public:
    using output_type = typename TypeConversionImpl::output_type;

    output_type to_output_format(std::string& str) {
        return m_impl.to_output_format(str);
    }
};

using StringConversion = TypeConversion<StringConversionImpl>;

using Int64Conversion = TypeConversion<Int64ConversionImpl>;


#endif /* SRC_TYPE_CONVERSION_HPP_ */
