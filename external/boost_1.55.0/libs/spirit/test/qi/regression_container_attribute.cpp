//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2011 Joerg Becker 
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// compile test only

#include <boost/spirit/include/qi.hpp>
#include <string>

int main() 
{
  namespace qi = boost::spirit::qi;

  qi::rule < std::string::const_iterator, std::string() > const t =
    "s" >> qi::attr( std::string() );

  boost::spirit::qi::symbols< char, std::string > keywords;
  keywords.add( "keyword", std::string( "keyword" ) );
  qi::rule < std::string::const_iterator, std::string() > const u =
    qi::lexeme[keywords >> !( qi::alnum | '_' )] >> qi::attr( std::string() );
}
