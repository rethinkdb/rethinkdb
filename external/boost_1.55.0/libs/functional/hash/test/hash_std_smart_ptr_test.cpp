
// Copyright 2012 Daniel James.
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

#if defined(BOOST_HASH_TEST_EXTENSIONS) && !defined(BOOST_NO_CXX11_SMART_PTR)
#define TEST_SMART_PTRS
#include <memory>
#endif

#ifdef TEST_SMART_PTRS

void shared_ptr_tests()
{
    std::shared_ptr<int> x;
    compile_time_tests(&x);

    BOOST_HASH_TEST_NAMESPACE::hash<std::shared_ptr<int> > x1;
    BOOST_HASH_TEST_NAMESPACE::hash<std::shared_ptr<int> > x2;

    std::shared_ptr<int> ptr1(new int(10));
    std::shared_ptr<int> ptr2;

    BOOST_TEST(x1(x) == x2(ptr2));
    BOOST_TEST(x1(x) != x2(ptr1));
    ptr2.reset(new int(10));
    BOOST_TEST(x1(ptr1) == x2(ptr1));
    BOOST_TEST(x1(ptr1) != x2(ptr2));
    ptr2 = ptr1;
    BOOST_TEST(x1(ptr1) == x2(ptr2));
#if defined(BOOST_HASH_TEST_EXTENSIONS)
    BOOST_TEST(x1(x) == BOOST_HASH_TEST_NAMESPACE::hash_value(x));
    BOOST_TEST(x1(ptr1) == BOOST_HASH_TEST_NAMESPACE::hash_value(ptr2));
#endif
}

void unique_ptr_tests()
{
    std::unique_ptr<int> x;
    compile_time_tests(&x);

    BOOST_HASH_TEST_NAMESPACE::hash<std::unique_ptr<int> > x1;
    BOOST_HASH_TEST_NAMESPACE::hash<std::unique_ptr<int> > x2;

    std::unique_ptr<int> ptr1(new int(10));
    std::unique_ptr<int> ptr2;

    BOOST_TEST(x1(x) == x2(ptr2));
    BOOST_TEST(x1(x) != x2(ptr1));
    ptr2.reset(new int(10));
    BOOST_TEST(x1(ptr1) == x2(ptr1));
    BOOST_TEST(x1(ptr1) != x2(ptr2));

#if defined(BOOST_HASH_TEST_EXTENSIONS)
    BOOST_TEST(x1(x) == BOOST_HASH_TEST_NAMESPACE::hash_value(x));
#endif
}

#endif

int main()
{
#ifdef TEST_SMART_PTRS
    shared_ptr_tests();
    unique_ptr_tests();
#endif

    return boost::report_errors();
}
