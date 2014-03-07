//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The purpose of this example is to show how a simple custom primitive parser
//  component can be written. We develop a custom parser exposing the current 
//  iterator position as its attribute.
//
//  For more information see: http://spirit.sourceforge.net/home/?page_id=567

#include <boost/spirit/include/qi_parse_attr.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/repository/include/qi_iter_pos.hpp>

#include <string>

namespace qi = boost::spirit::qi;

int main()
{
    using boost::spirit::repository::qi::iter_pos;

    std::string prefix, suffix;           // attributes receiving the
    std::string::iterator position;       // parsed values

    std::string input("prefix1234567");
    std::string::iterator first = input.begin();
    bool result = 
        qi::parse(first, input.end()
          , +qi::alpha >> iter_pos >> +qi::digit
          , prefix, position, suffix);

    if (result) 
    {
        std::cout << "-------------------------------- \n";
        std::cout << "Parsing succeeded\n";
        std::cout << "prefix is: " << prefix << "\n";
        std::cout << "suffix is: " << suffix << "\n";
        std::cout << "position is: " << std::distance(input.begin(), position) << "\n";
        std::cout << "-------------------------------- \n";
    }
    else 
    {
        std::cout << "-------------------------------- \n";
        std::cout << "Parsing failed\n";
        std::cout << "-------------------------------- \n";
    }
    return 0;
}
