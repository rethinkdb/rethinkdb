
// Copyright 2012 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <cmath>

namespace test
{
    template <class T1>
    struct check_return_type
    {
        template <class T2>
        static void equals(T2)
        {
            BOOST_STATIC_ASSERT((boost::is_same<T1, T2>::value));
        }

        template <class T2>
        static void equals_ref(T2&)
        {
            BOOST_STATIC_ASSERT((boost::is_same<T1, T2>::value));
        }

        template <class T2>
        static void convertible(T2)
        {
            BOOST_STATIC_ASSERT((boost::is_convertible<T2, T1>::value));
        }
    };
}

int main() {
    float f = 0;
    double d = 0;
    long double l = 0;

    test::check_return_type<float>::equals(std::ldexp(f, 0));
    test::check_return_type<double>::equals(std::ldexp(d, 0));
    test::check_return_type<long double>::equals(std::ldexp(l, 0));

    int dummy = 0;

    test::check_return_type<float>::equals(std::frexp(f, &dummy));
    test::check_return_type<double>::equals(std::frexp(d, &dummy));
    test::check_return_type<long double>::equals(std::frexp(l, &dummy));

#if BOOST_HASH_USE_FPCLASSIFY

    int (*fpc1)(float) = std::fpclassify;
    int (*fpc2)(double) = std::fpclassify;
    int (*fpc3)(long double) = std::fpclassify;

#endif
}
