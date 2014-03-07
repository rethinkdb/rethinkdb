
//  (C) Copyright Dave Abrahams 2003. Use, modification and distribution is
//  subject to the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_enum.hpp>
#endif

struct convertible_to_anything
{
    template<typename T> operator T() { return 0; }
};


TT_TEST_BEGIN(is_enum)

BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_enum<convertible_to_anything>::value, false);
BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_enum<int[] >::value, false);

TT_TEST_END


