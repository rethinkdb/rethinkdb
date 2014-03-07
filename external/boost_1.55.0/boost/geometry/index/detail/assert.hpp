// Boost.Geometry Index
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_ASSERT_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_ASSERT_HPP

#include <boost/assert.hpp>

#define BOOST_GEOMETRY_INDEX_ASSERT(CONDITION, TEXT_MSG) \
    BOOST_ASSERT_MSG(CONDITION, TEXT_MSG)

// TODO - change it to something like:
// BOOST_ASSERT((CONDITION) && (TEXT_MSG))

#if defined(BOOST_DISABLE_ASSERTS) || defined(NDEBUG)

#define BOOST_GEOMETRY_INDEX_ASSERT_UNUSED_PARAM(PARAM)

#else

#define BOOST_GEOMETRY_INDEX_ASSERT_UNUSED_PARAM(PARAM) PARAM

#endif

#endif // BOOST_GEOMETRY_INDEX_DETAIL_ASSERT_HPP
