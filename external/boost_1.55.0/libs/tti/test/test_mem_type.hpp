
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(TEST_MEMBER_TYPE_HPP)
#define TEST_MEMBER_TYPE_HPP

#include "test_structs.hpp"
#include <boost/tti/member_type.hpp>

BOOST_TTI_MEMBER_TYPE(AnIntType)
BOOST_TTI_TRAIT_MEMBER_TYPE(NameStruct,AStructType)
BOOST_TTI_MEMBER_TYPE(AnIntTypeReference)
BOOST_TTI_MEMBER_TYPE(BType)
BOOST_TTI_TRAIT_MEMBER_TYPE(TheInteger,AnIntegerType)
BOOST_TTI_MEMBER_TYPE(CType)
BOOST_TTI_MEMBER_TYPE(AnotherIntegerType)
BOOST_TTI_TRAIT_MEMBER_TYPE(SomethingElse,someOtherType)

BOOST_TTI_MEMBER_TYPE(NoExistType)

#endif // TEST_MEMBER_TYPE_HPP
