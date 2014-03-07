
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(TEST_HAS_STATIC_MEMBER_DATA_HPP)
#define TEST_HAS_STATIC_MEMBER_DATA_HPP

#include "test_structs.hpp"
#include <boost/tti/has_static_member_data.hpp>

BOOST_TTI_HAS_STATIC_MEMBER_DATA(DSMember)
BOOST_TTI_TRAIT_HAS_STATIC_MEMBER_DATA(StatName,AnStat)
BOOST_TTI_HAS_STATIC_MEMBER_DATA(SomeStaticData)
BOOST_TTI_HAS_STATIC_MEMBER_DATA(CIntValue)

#endif // TEST_HAS_STATIC_MEMBER_DATA_HPP
