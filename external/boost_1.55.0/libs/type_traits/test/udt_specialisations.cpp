
//  (C) Copyright John Maddock 2004. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_pod.hpp>
#  include <boost/type_traits/is_class.hpp>
#  include <boost/type_traits/is_union.hpp>
#endif

struct my_pod{};
struct my_union
{
   char c;
   int i;
};

namespace tt
{
template<>
struct is_pod<my_pod> 
   : public mpl::true_{};
template<>
struct is_pod<my_union> 
   : public mpl::true_{};
template<>
struct is_union<my_union> 
   : public mpl::true_{};
template<>
struct is_class<my_union> 
   : public mpl::false_{};
}

TT_TEST_BEGIN(is_pod)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<my_pod>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_pod<my_union>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_union<my_union>::value, true);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_class<my_union>::value, false);

TT_TEST_END

