// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <geometry_test_common.hpp>

#include <boost/geometry/util/select_most_precise.hpp>


struct user_defined {};

template <typename T1, typename T2, typename ExpectedType>
void test()
{
    typedef typename bg::select_most_precise<T1, T2>::type type;

    BOOST_CHECK((boost::is_same<type, ExpectedType>::type::value));
}

int test_main(int, char* [])
{
    // fp only
    test<float, float, float>();
    test<float, double, double>();
    test<double, float, double>();
    test<double, double, double>();

    // integer only
    test<int, int, int>();
    test<int, short int, int>();
    test<short int, int, int>();
    test<short int, short int, short int>();

    // int/fp
    test<double, int, double>();
    test<int, double, double>();
    test<long double, long double, long double>();
    test<float, int, float>();
    test<int, float, float>();

#ifndef _MSC_VER
    // This cannot be done for MSVC because double/long double is the same
    test<double, long double, long double>();
    test<long double, double, long double>();
#endif

    // with any other non-integer/float class
    test<int, user_defined, user_defined>();
    test<user_defined, int, user_defined>();
    test<long double, user_defined, user_defined>();
    test<user_defined, long double, user_defined>();
    test<user_defined, user_defined, user_defined>();

    // should not compile
    //test<void, void, void>();

    return 0;
}
