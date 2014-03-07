
// Copyright 2012 Daniel James.
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

#include <boost/config.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined(BOOST_HASH_TEST_EXTENSIONS) && !defined(BOOST_NO_CXX11_HDR_ARRAY)
#define TEST_ARRAY
#include <array>
#include <vector>
#endif

#ifdef TEST_ARRAY

template <typename T>
void array_tests(T const& v) {
    boost::hash<typename T::value_type> hf;
    for(typename T::const_iterator i = v.begin(); i != v.end(); ++i) {
        for(typename T::const_iterator j = v.begin(); j != v.end(); ++j) {
            if (i != j)
                BOOST_TEST(hf(*i) != hf(*j));
            else
                BOOST_TEST(hf(*i) == hf(*j));
        }
    }
}

void empty_array_test() {
/*
    boost::hash<std::array<int, 0> > empty_array_hash;
    std::array<int, 0> empty_array;
    BOOST_TEST(empty_array_hash(empty_array) == boost::hash_value(empty_array));
*/
}

void int_1_array_test()
{
    std::vector<std::array<int, 1> > arrays;
    std::array<int, 1> val;
    val[0] = 0;
    arrays.push_back(val);
    val[0] = 1;
    arrays.push_back(val);
    val[0] = 2;
    arrays.push_back(val);
    array_tests(arrays);
}

void string_1_array_test()
{
    std::vector<std::array<std::string, 1> > arrays;
    std::array<std::string, 1> val;
    arrays.push_back(val);
    val[0] = "one";
    arrays.push_back(val);
    val[0] = "two";
    arrays.push_back(val);
    array_tests(arrays);
}

void string_3_array_test()
{
    std::vector<std::array<std::string,3 > > arrays;
    std::array<std::string, 3> val;
    arrays.push_back(val);
    val[0] = "one";
    arrays.push_back(val);
    val[0] = ""; val[1] = "one"; val[2] = "";
    arrays.push_back(val);
    val[0] = ""; val[1] = ""; val[2] = "one";
    arrays.push_back(val);
    val[0] = "one"; val[1] = "one"; val[2] = "one";
    arrays.push_back(val);
    val[0] = "one"; val[1] = "two"; val[2] = "three";
    arrays.push_back(val);
    array_tests(arrays);
}

#endif // TEST_ARRAY

int main()
{
#ifdef TEST_ARRAY
    empty_array_test();
    int_1_array_test();
    string_1_array_test();
    string_3_array_test();
#endif

    return boost::report_errors();
}
