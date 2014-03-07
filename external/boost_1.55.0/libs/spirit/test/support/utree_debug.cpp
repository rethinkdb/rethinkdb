// Copyright (c) 2001-2011 Hartmut Kaiser
// 
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>

#define BOOST_SPIRIT_DEBUG 1

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_utree.hpp>

#include <string>

namespace qi = boost::spirit::qi;
namespace spirit = boost::spirit;

int main()
{
    qi::rule<std::string::iterator, spirit::utree()> r = qi::int_;
    BOOST_SPIRIT_DEBUG_NODE(r);

    spirit::utree ut;
    std::string input("1");
    BOOST_TEST(qi::parse(input.begin(), input.end(), r, ut));
    BOOST_TEST(ut.which() == spirit::utree_type::int_type && ut.get<int>() == 1);

    return boost::report_errors();
}
