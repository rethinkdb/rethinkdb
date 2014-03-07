/*==============================================================================
    Copyright (c) 2011 Steven Watanabe

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_PHOENIX_CMATH_HPP_INCLUDED
#define BOOST_PHOENIX_CMATH_HPP_INCLUDED

#include <boost/phoenix/core/limits.hpp>
#include <cmath>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>

namespace boost {

#define BOOST_PHOENIX_MATH_FUNCTION(name, n)                            \
    namespace phoenix_impl {                                            \
    struct name ## _impl {                                              \
        template<class Sig>                                             \
        struct result;                                                  \
        template<class This, BOOST_PHOENIX_typename_A(n)>               \
        struct result<This(BOOST_PHOENIX_A(n))>                         \
        {                                                               \
            typedef                                                     \
                typename proto::detail::uncvref<A0>::type               \
                type;                                                   \
        };                                                              \
        template<BOOST_PHOENIX_typename_A(n)>                           \
        A0 operator()(BOOST_PHOENIX_A_const_ref_a(n)) const {           \
            using namespace std;                                        \
            return name(BOOST_PHOENIX_a(n));                            \
        }                                                               \
    };                                                                  \
    }                                                                   \
    namespace phoenix {                                                 \
    BOOST_PHOENIX_ADAPT_CALLABLE(name, phoenix_impl::name ## _impl, 1)  \
    }

BOOST_PHOENIX_MATH_FUNCTION(acos, 1)
BOOST_PHOENIX_MATH_FUNCTION(asin, 1)
BOOST_PHOENIX_MATH_FUNCTION(atan, 1)
BOOST_PHOENIX_MATH_FUNCTION(atan2, 2)
BOOST_PHOENIX_MATH_FUNCTION(ceil, 1)
BOOST_PHOENIX_MATH_FUNCTION(cos, 1)
BOOST_PHOENIX_MATH_FUNCTION(cosh, 1)
BOOST_PHOENIX_MATH_FUNCTION(exp, 1)
BOOST_PHOENIX_MATH_FUNCTION(fabs, 1)
BOOST_PHOENIX_MATH_FUNCTION(floor, 1)
BOOST_PHOENIX_MATH_FUNCTION(fmod, 2)
BOOST_PHOENIX_MATH_FUNCTION(frexp, 2)
BOOST_PHOENIX_MATH_FUNCTION(ldexp, 2)
BOOST_PHOENIX_MATH_FUNCTION(log, 1)
BOOST_PHOENIX_MATH_FUNCTION(log10, 1)
BOOST_PHOENIX_MATH_FUNCTION(modf, 2)
BOOST_PHOENIX_MATH_FUNCTION(pow, 2)
BOOST_PHOENIX_MATH_FUNCTION(sin, 1)
BOOST_PHOENIX_MATH_FUNCTION(sinh, 1)
BOOST_PHOENIX_MATH_FUNCTION(sqrt, 1)
BOOST_PHOENIX_MATH_FUNCTION(tan, 1)
BOOST_PHOENIX_MATH_FUNCTION(tanh, 1)

#undef BOOST_PHOENIX_MATH_FUNCTION

}

#endif
