
//  (C) Copyright Edward Diener 2012
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(TEST_HAS_DATA_HPP)
#define TEST_HAS_DATA_HPP

#include "test_structs.hpp"
#include <boost/tti/has_data.hpp>

BOOST_TTI_HAS_DATA(AnInt)
BOOST_TTI_HAS_DATA(DSMember)
BOOST_TTI_HAS_DATA(aMember)
BOOST_TTI_TRAIT_HAS_DATA(DCMember,cMem)
BOOST_TTI_HAS_DATA(someDataMember)
BOOST_TTI_HAS_DATA(CIntValue)
BOOST_TTI_HAS_DATA(SomeStaticData)
BOOST_TTI_TRAIT_HAS_DATA(DStatName,AnStat)
BOOST_TTI_HAS_DATA(IntBT)
BOOST_TTI_TRAIT_HAS_DATA(DNestedData,NestedCT)
BOOST_TTI_TRAIT_HAS_DATA(DAOther,OtherAT)
BOOST_TTI_HAS_DATA(ONestStr)

#endif // TEST_HAS_DATA_HPP
