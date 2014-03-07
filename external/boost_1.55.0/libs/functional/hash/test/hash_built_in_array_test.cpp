
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

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
    const int length1 = 25;
    int array1[25] = {
        26, -43, 32, 65, 45,
        12, 67, 32, 12, 23,
        0, 0, 0, 0, 0,
        8, -12, 23, 65, 45,
        -1, 93, -54, 987, 3
    };
    BOOST_HASH_TEST_NAMESPACE::hash<int[25]> hasher1;

    const int length2 = 1;
    int array2[1] = {3};
    BOOST_HASH_TEST_NAMESPACE::hash<int[1]> hasher2;

    const int length3 = 2;
    int array3[2] = {2, 3};
    BOOST_HASH_TEST_NAMESPACE::hash<int[2]> hasher3;

    BOOST_TEST(hasher1(array1)
            == BOOST_HASH_TEST_NAMESPACE::hash_range(array1, array1 + length1));
    BOOST_TEST(hasher2(array2)
            == BOOST_HASH_TEST_NAMESPACE::hash_range(array2, array2 + length2));
    BOOST_TEST(hasher3(array3)
            == BOOST_HASH_TEST_NAMESPACE::hash_range(array3, array3 + length3));
}

void two_dimensional_array_test()
{
    int array[3][2] = {{-5, 6}, {7, -3}, {26, 1}};
    BOOST_HASH_TEST_NAMESPACE::hash<int[3][2]> hasher;

    std::size_t seed1 = 0;
    for(int i = 0; i < 3; ++i)
    {
        std::size_t seed2 = 0;
        for(int j = 0; j < 2; ++j)
            BOOST_HASH_TEST_NAMESPACE::hash_combine(seed2, array[i][j]);
        BOOST_HASH_TEST_NAMESPACE::hash_combine(seed1, seed2);
    }

    BOOST_TEST(hasher(array) == seed1);
    BOOST_TEST(hasher(array) == BOOST_HASH_TEST_NAMESPACE::hash_range(array, array + 3));
}

#endif // BOOST_HASH_TEST_EXTENSIONS

int main()
{
#ifdef BOOST_HASH_TEST_EXTENSIONS
    array_int_test();
    two_dimensional_array_test();
#endif
    return boost::report_errors();
}
