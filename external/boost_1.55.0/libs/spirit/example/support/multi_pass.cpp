//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <fstream>
#include <vector>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//[tutorial_multi_pass
int main()
{
    namespace spirit = boost::spirit;
    using spirit::ascii::space;
    using spirit::ascii::char_;
    using spirit::qi::double_;
    using spirit::qi::eol;

    std::ifstream in("multi_pass.txt");    // we get our input from this file
    if (!in.is_open()) {
        std::cout << "Could not open input file: 'multi_pass.txt'" << std::endl;
        return -1;
    }

    typedef std::istreambuf_iterator<char> base_iterator_type;
    spirit::multi_pass<base_iterator_type> first = 
        spirit::make_default_multi_pass(base_iterator_type(in));

    std::vector<double> v;
    bool result = spirit::qi::phrase_parse(first
      , spirit::make_default_multi_pass(base_iterator_type())
      , double_ >> *(',' >> double_)              // recognize list of doubles
      , space | '#' >> *(char_ - eol) >> eol      // comment skipper
      , v);                                       // data read from file

    if (!result) {
        std::cout << "Failed parsing input file!" << std::endl;
        return -2;
    }

    std::cout << "Successfully parsed input file!" << std::endl;
    return 0;
}
//]
