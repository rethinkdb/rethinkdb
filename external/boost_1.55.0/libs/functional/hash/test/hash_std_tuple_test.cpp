
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

#if defined(BOOST_HASH_TEST_EXTENSIONS) && !defined(BOOST_NO_CXX11_HDR_TUPLE)
#define TEST_TUPLE
#include <tuple>
#include <vector>
#endif

#ifdef TEST_TUPLE

template <typename T>
void tuple_tests(T const& v) {
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

void empty_tuple_test() {
    boost::hash<std::tuple<> > empty_tuple_hash;
    std::tuple<> empty_tuple;
    BOOST_TEST(empty_tuple_hash(empty_tuple) == boost::hash_value(empty_tuple));
}

void int_tuple_test() {
    std::vector<std::tuple<int> > int_tuples;
    int_tuples.push_back(std::make_tuple(0));
    int_tuples.push_back(std::make_tuple(1));
    int_tuples.push_back(std::make_tuple(2));
    tuple_tests(int_tuples);
}

void int_string_tuple_test() {
    std::vector<std::tuple<int, std::string> > int_string_tuples;
    int_string_tuples.push_back(std::make_tuple(0, std::string("zero")));
    int_string_tuples.push_back(std::make_tuple(1, std::string("one")));
    int_string_tuples.push_back(std::make_tuple(2, std::string("two")));
    int_string_tuples.push_back(std::make_tuple(0, std::string("one")));
    int_string_tuples.push_back(std::make_tuple(1, std::string("zero")));
    int_string_tuples.push_back(std::make_tuple(0, std::string("")));
    int_string_tuples.push_back(std::make_tuple(1, std::string("")));
    tuple_tests(int_string_tuples);
}

#endif // TEST_TUPLE

int main()
{
#ifdef TEST_TUPLE
    empty_tuple_test();
    int_tuple_test();
    int_string_tuple_test();
#endif

    return boost::report_errors();
}
