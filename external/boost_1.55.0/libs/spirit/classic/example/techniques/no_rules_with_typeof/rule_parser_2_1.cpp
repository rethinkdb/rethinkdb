/*==============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
//------------------------------------------------------------------------------
//  This example shows a recursive grammar built using subrules and typeof.
//  See boost/spirit/include/rule_parser.hpp for details.
//  This example is based on subrule_calc.cpp.
//------------------------------------------------------------------------------
#include <string>
#include <iostream>

#include <boost/typeof/typeof.hpp>

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_typeof.hpp>

#include <boost/spirit/include/classic_rule_parser.hpp>

// Don't forget to
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

using namespace BOOST_SPIRIT_CLASSIC_NS;

// Semantic actions
namespace 
{
  void do_int(int v)       { std::cout << "PUSH(" << v << ')' << std::endl; }
  void do_add(char const*, char const*)  { std::cout << "ADD" << std::endl; }
  void do_sub(char const*, char const*)  { std::cout << "SUB" << std::endl; }
  void do_mul(char const*, char const*)  { std::cout << "MUL" << std::endl; }
  void do_div(char const*, char const*)  { std::cout << "DIV" << std::endl; }
  void do_neg(char const*, char const*)  { std::cout << "NEG" << std::endl; }
}

// Operating at root namespace...
#define BOOST_SPIRIT__NAMESPACE -


// Our calculator grammar using subrules in a rule parser.
BOOST_SPIRIT_RULE_PARSER( calc,
  -,
  -,
  (3,( ((subrule<0>),expression,()),
       ((subrule<1>),term,()),
       ((subrule<2>),factor,() )) ),
  (
    expression =
        term
        >> *(   ('+' >> term)[&do_add]
            |   ('-' >> term)[&do_sub]
            )
    ,

    term =
        factor
        >> *(   ('*' >> factor)[&do_mul]
            |   ('/' >> factor)[&do_div]
            )
    ,

    factor =   
        int_p[&do_int]
        |   ('(' >> expression >> ')')
        |   ('-' >> factor)[&do_neg]
        |   ('+' >> factor)
  )
)

#undef BOOST_SPIRIT__NAMESPACE 


// Main program
int main()
{
  std::cout 
  << "/////////////////////////////////////////////////////////\n"
  << "\t\tA ruleless calculator using subrules...\n"
  << "/////////////////////////////////////////////////////////\n"
  << "Type an expression...or an empty line to quit\n" 
  << std::endl;

  std::string str;
  while (std::getline(std::cin, str))
  {
    if (str.empty()) break;

    parse_info<> info = parse(str.c_str(), calc, space_p);

    if (info.full)
      std::cout 
      << "OK." 
      << std::endl;
    else
      std::cout 
      << "ERROR.\n"
      << "Stopped at: \": " << info.stop << "\".\n"
      << std::endl;
  }
  return 0;
}

