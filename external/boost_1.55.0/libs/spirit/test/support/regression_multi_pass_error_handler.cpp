//  Copyright (c) 2001-2010 Hartmut Kaiser
//  Copyright (c) 2011 Laurent Gomila 
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <sstream>
#include <iostream>
#include <iterator>
#include <string>

using namespace boost::spirit;
using namespace boost;

int main()
{
    {
        std::string input("5x");
        std::istringstream iss(input);

        typedef std::istreambuf_iterator<char> base_iterator_type;
        typedef multi_pass<base_iterator_type> iterator_type;

        iterator_type first = make_default_multi_pass(base_iterator_type(iss));
        iterator_type last  = make_default_multi_pass(base_iterator_type());

        std::ostringstream oss;

        qi::rule<iterator_type> r = qi::int_ > qi::int_;
        qi::on_error<qi::fail>(r, phoenix::ref(oss) << phoenix::val("error"));

        BOOST_TEST(!qi::parse(first, last, r));
        BOOST_TEST(oss.str() == "error");
    }

    {
        std::string input("5x");
        std::istringstream iss(input);

        typedef std::istreambuf_iterator<char> base_iterator_type;
        typedef multi_pass<base_iterator_type> iterator_type;

        iterator_type first = make_default_multi_pass(base_iterator_type(iss));
        iterator_type last  = make_default_multi_pass(base_iterator_type());

        std::ostringstream oss;

        qi::rule<iterator_type> r1 = qi::int_ > qi::int_;
        qi::rule<iterator_type> r2 = qi::int_ > qi::char_;
        qi::on_error<qi::fail>(r1, phoenix::ref(oss) << phoenix::val("error in r1"));
        qi::on_error<qi::fail>(r2, phoenix::ref(oss) << phoenix::val("error in r2"));

        BOOST_TEST(qi::parse(first, last, r1 | r2));
        BOOST_TEST(oss.str() == "error in r1");
    }

    return boost::report_errors();
}

