//  Copyright (c) 2010 Daniel James
//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/qi.hpp>
#include <vector>

int main() 
{
    typedef char const* Iterator;
    namespace qi = boost::spirit::qi;

    qi::rule<Iterator, std::vector<boost::iterator_range<Iterator> >()> list;
    list = *qi::raw[qi::char_]; // This fails to compile
    
    char const* test = "abcdef";
    unsigned test_length = 6;
    char const* test_begin = test;
    char const* test_end = test + test_length;
    std::vector<boost::iterator_range<Iterator> > result;
    bool r = qi::parse(test_begin, test_end, list, result);
    
    BOOST_TEST(r);
    BOOST_TEST(test_begin == test_end);
    BOOST_TEST(result.size() == test_length);

    for(unsigned i = 0; i < test_length; ++i) {
        BOOST_TEST(result[i].begin() == test + i);
        BOOST_TEST(result[i].end() == test + i + 1);
    }

    return boost::report_errors();
}
