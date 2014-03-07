
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
#include "./compile_time.hpp"

void void_func1() { static int x = 1; ++x; }
void void_func2() { static int x = 2; --x; }
int int_func1(int) { return 0; }
int int_func2(int) { return 1; }

void function_pointer_tests()
{
    compile_time_tests((void(**)()) 0);
    compile_time_tests((int(**)(int)) 0);

    BOOST_HASH_TEST_NAMESPACE::hash<void(*)()> hasher_void;
    BOOST_HASH_TEST_NAMESPACE::hash<int(*)(int)> hasher_int;

    BOOST_TEST(&void_func1 != &void_func2);
    BOOST_TEST(&int_func1 != &int_func2);

    BOOST_TEST(hasher_void(0) == hasher_void(0));
    BOOST_TEST(hasher_void(&void_func1) == hasher_void(&void_func1));
    BOOST_TEST(hasher_void(&void_func1) != hasher_void(&void_func2));
    BOOST_TEST(hasher_void(&void_func1) != hasher_void(0));
    BOOST_TEST(hasher_int(0) == hasher_int(0));
    BOOST_TEST(hasher_int(&int_func1) == hasher_int(&int_func1));
    BOOST_TEST(hasher_int(&int_func1) != hasher_int(&int_func2));
    BOOST_TEST(hasher_int(&int_func1) != hasher_int(0));
#if defined(BOOST_HASH_TEST_EXTENSIONS)
    BOOST_TEST(hasher_void(&void_func1)
            == BOOST_HASH_TEST_NAMESPACE::hash_value(&void_func1));
    BOOST_TEST(hasher_int(&int_func1)
            == BOOST_HASH_TEST_NAMESPACE::hash_value(&int_func1));

    // This isn't specified in Peter's proposal:
    BOOST_TEST(hasher_void(0) == 0);
#endif
}

int main()
{
    function_pointer_tests();

    return boost::report_errors();
}
