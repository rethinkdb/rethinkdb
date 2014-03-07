//  Copyright (c) 2010 Daniel James
//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// this is a compile only regression test 

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;

struct source_mode {};

struct process_type
{
    template <typename A, typename B, typename C>
    void operator()(A&, B&, C&) const {}
};

int main() 
{
    process_type process;
    qi::rule<char const*> x = qi::attr(source_mode()) [process];
    return 0;
}
