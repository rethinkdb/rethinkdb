
// Copyright 2011 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./config.hpp"

#ifdef BOOST_HASH_TEST_STD_INCLUDES
#  include <functional>
#else
#  include <boost/functional/hash.hpp>
#endif
#include <boost/config.hpp>
#include <boost/detail/lightweight_test.hpp>

#if !defined(BOOST_NO_CXX11_HDR_TYPEINDEX)

#include <typeindex>

void test_type_index() {
    BOOST_HASH_TEST_NAMESPACE::hash<std::type_index> hasher;

#if defined(BOOST_NO_TYPEID)
    std::cout<<"Unable to test std::type_index, as typeid isn't available"
        <<std::endl;
#else
    std::type_index int_index = typeid(int);
    std::type_index int2_index = typeid(int);
    std::type_index char_index = typeid(char);
    
    BOOST_TEST(hasher(int_index) == int_index.hash_code());
    BOOST_TEST(hasher(int_index) == int2_index.hash_code());
    BOOST_TEST(hasher(char_index) == char_index.hash_code());
    BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(int_index) == int_index.hash_code());
    BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(int_index) == int2_index.hash_code());
    BOOST_TEST(BOOST_HASH_TEST_NAMESPACE::hash_value(char_index) == char_index.hash_code());

    BOOST_TEST(hasher(int_index) == hasher(int2_index));
    BOOST_TEST(hasher(int_index) != hasher(char_index));
#endif
}
#endif

int main()
{
#if !defined(BOOST_NO_CXX11_HDR_TYPEINDEX)
    test_type_index();
#else
    std::cout<<"<type_index> not available."<<std::endl;
#endif

    return boost::report_errors();
}
