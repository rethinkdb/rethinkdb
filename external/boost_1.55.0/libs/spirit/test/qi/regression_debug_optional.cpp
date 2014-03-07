//  Copyright (c) 2010 Carl Philipp Reh
//  Copyright (c) 2001-2010 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  make sure optionals play well with debug output

#define BOOST_SPIRIT_DEBUG
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/home/support/attributes.hpp>
#include <boost/optional.hpp>

int main()
{
    boost::spirit::qi::rule<
        char const *,
        boost::optional<int>()
    > foo;

    BOOST_SPIRIT_DEBUG_NODE(foo);
    return boost::report_errors();
}
