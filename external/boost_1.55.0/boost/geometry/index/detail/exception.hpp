// Boost.Geometry Index
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_EXCEPTION_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_EXCEPTION_HPP

#ifndef BOOST_NO_EXCEPTIONS
#include <stdexcept>
#else
#include <cstdlib>
#endif

namespace boost { namespace geometry { namespace index { namespace detail {

#ifndef BOOST_NO_EXCEPTIONS

inline void throw_runtime_error(const char * str)
{
    throw std::runtime_error(str);
}

inline void throw_logic_error(const char * str)
{
    throw std::logic_error(str);
}

inline void throw_invalid_argument(const char * str)
{
    throw std::invalid_argument(str);
}

inline void throw_length_error(const char * str)
{
    throw std::length_error(str);
}

#else

inline void throw_runtime_error(const char * str)
{
    BOOST_ASSERT_MSG(!"runtime_error thrown", str);
    std::abort();
}

inline void throw_logic_error(const char * str)
{
    BOOST_ASSERT_MSG(!"logic_error thrown", str);
    std::abort();
}

inline void throw_invalid_argument(const char * str)
{
    BOOST_ASSERT_MSG(!"invalid_argument thrown", str);
    std::abort();
}

inline void throw_length_error(const char * str)
{
    BOOST_ASSERT_MSG(!"length_error thrown", str);
    std::abort();
}

#endif

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_EXCEPTION_HPP
