
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// On some compilers hash_value isn't available for arrays, so I test it
// separately from the main array tests.

#include "./config.hpp"

#ifdef BOOST_HASH_TEST_EXTENSIONS
#  ifdef BOOST_HASH_TEST_STD_INCLUDES
#    include <functional>
#  else
#    include <boost/functional/hash.hpp>
#  endif
#endif

#include <boost/detail/lightweight_test.hpp>

#ifdef BOOST_HASH_TEST_EXTENSIONS

void array_int_test()
{
    const int array1[25] = {
        26, -43, 32, 65, 45,
        12, 67, 32, 12, 23,
        0, 0, 0, 0, 0,
        8, -12, 23, 65, 45,
        -1, 93, -54, 987, 3
    };
    BOOST_HASH_TEST_NAMESPACE::hash<int[25]> hasher1;

    int array2[1] = {3};
    BOOST_HASH_TEST_NAMESPACE::hash<int[1]> hasher2;

    int array3[2] = {2, 3};
    BOOST_HASH_TEST_NAMESPACE::hash<int[2]> hasher3;

    BOOST_TEST(hasher1(array1) == BOOST_HASH_TEST_NAMESPACE::hash_value(array1));
    BOOST_TEST(hasher2(array2) == BOOST_HASH_TEST_NAMESPACE::hash_value(array2));
    BOOST_TEST(hasher3(array3) == BOOST_HASH_TEST_NAMESPACE::hash_value(array3));
}

void two_dimensional_array_test()
{
    int array[3][2] = {{-5, 6}, {7, -3}, {26, 1}};
    BOOST_HASH_TEST_NAMESPACE::hash<int[3][2]> hasher;

    BOOST_TEST(hasher(array) == BOOST_HASH_TEST_NAMESPACE::hash_value(array));
}

#endif

int main()
{
#ifdef BOOST_HASH_TEST_EXTENSIONS
    array_int_test();
    two_dimensional_array_test();
#endif

    return boost::report_errors();
}

