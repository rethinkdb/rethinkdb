
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./config.hpp"

#ifdef BOOST_HASH_TEST_STD_INCLUDES
#  include <functional>
#else
#  include <boost/functional/hash.hpp>
#endif

#include <boost/detail/lightweight_test.hpp>
#include <boost/limits.hpp>
#include "./compile_time.hpp"

void pointer_tests()
{
    compile_time_tests((int**) 0);
    compile_time_tests((void**) 0);

    BOOST_HASH_TEST_NAMESPACE::hash<int*> x1;
    BOOST_HASH_TEST_NAMESPACE::hash<int*> x2;

    int int1;
    int int2;

    BOOST_TEST(x1(0) == x2(0));
    BOOST_TEST(x1(&int1) == x2(&int1));
    BOOST_TEST(x1(&int2) == x2(&int2));
#if defined(BOOST_HASH_TEST_EXTENSIONS)
    BOOST_TEST(x1(&int1) == BOOST_HASH_TEST_NAMESPACE::hash_value(&int1));
    BOOST_TEST(x1(&int2) == BOOST_HASH_TEST_NAMESPACE::hash_value(&int2));

    // This isn't specified in Peter's proposal:
    BOOST_TEST(x1(0) == 0);
#endif
}

int main()
{
    pointer_tests();
    return boost::report_errors();
}
