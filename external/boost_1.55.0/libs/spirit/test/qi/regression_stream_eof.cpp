//   Copyright (c) 2012 Louis Dionne
//   Copyright (c) 2001-2012 Hartmut Kaiser
//
//   Distributed under the Boost Software License, Version 1.0. (See accompanying
//   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/qi.hpp>

#include <iostream>
#include <string>

struct MyInt 
{
    int i_;

    template <typename Istream>
    friend Istream operator>>(Istream& is, MyInt& self) 
    {
        is >> self.i_;
        return is;
    }
};

int main()
{
    using namespace boost::spirit::qi;
    typedef std::string::const_iterator Iterator;

    std::string input = "1";
    Iterator first(input.begin()), last(input.end());
    rule<Iterator, int()> my_int = stream_parser<char, MyInt>();
    BOOST_TEST(parse(first, last, my_int));
    BOOST_TEST(first == last);

    return boost::report_errors();
}

