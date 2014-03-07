//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The purpose of this example is to demonstrate different use cases for the
//  distinct parser.

#include <iostream>
#include <string>
#include <vector>

//[qi_distinct_includes
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/repository/include/qi_distinct.hpp>
//]

//[qi_distinct_namespace
using namespace boost::spirit;
using namespace boost::spirit::ascii;
using boost::spirit::repository::distinct;
//]

int main()
{
    //[qi_distinct_description_ident
    {
        std::string str("description ident");
        std::string::iterator first(str.begin());
        bool r = qi::phrase_parse(first, str.end()
          , distinct(alnum | '_')["description"] >> -lit("--") >> +(alnum | '_')
          , space);
        BOOST_ASSERT(r && first == str.end());
    }
    //]

    //[qi_distinct_description__ident
    {
        std::string str("description--ident");
        std::string::iterator first(str.begin());
        bool r = qi::phrase_parse(first, str.end()
          , distinct(alnum | '_')["description"] >> -lit("--") >> +(alnum | '_')
          , space);
        BOOST_ASSERT(r && first == str.end());
    }
    //]

    //[qi_distinct_description_ident_error
    {
        std::string str("description-ident");
        std::string::iterator first(str.begin());
        bool r = qi::phrase_parse(first, str.end()
          , distinct(alnum | '_')["description"] >> -lit("--") >> +(alnum | '_')
          , space);
        BOOST_ASSERT(!r && first == str.begin());
    }
    //]

    return 0;
}
