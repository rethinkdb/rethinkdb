/*==============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
//------------------------------------------------------------------------------
//  This example uses typeof to build a nonrecursive grammar.
//  See boost/spirit/include/rule_parser.hpp for details.
//------------------------------------------------------------------------------
#include <string>
#include <iostream>

#include <boost/typeof/typeof.hpp>

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_typeof.hpp>

#include <boost/spirit/include/classic_confix.hpp>
#include <boost/spirit/include/classic_typeof.hpp>

#include <boost/spirit/include/classic_rule_parser.hpp>

// It's important to create an own registration group, even if there are no
// manual Typeof registrations like in this case.
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

using namespace BOOST_SPIRIT_CLASSIC_NS;

namespace my_project { namespace my_module {

  // Semantic actions.
  void echo_uint(unsigned i) { std::cout << i; }
  void comma(unsigned) { std::cout << ','; }

  #define BOOST_SPIRIT__NAMESPACE (2,(my_project,my_module))

  // Define the action placeholders we use.
  BOOST_SPIRIT_ACTION_PLACEHOLDER(uint_action)
  BOOST_SPIRIT_ACTION_PLACEHOLDER(next_action)

  // Parser for unsigned decimal, hexadecimal and binary literals.
  // Takes a function pointer as its parameter.
  BOOST_SPIRIT_RULE_PARSER(uint_literal
  ,-,(1,( uint_action )),-,

      str_p("0x") >> hex_p[ uint_action ]
    | str_p("0b") >> bin_p[ uint_action ]
    | uint_p[ uint_action ]
  ) 

  // Parser for a list of (dec/hex/bin) uints.
  BOOST_SPIRIT_RULE_PARSER(uint_list,
    -,(2,( uint_action, next_action )),-,

    uint_literal[uint_action] >> *(',' >> uint_literal[next_action][uint_action]) 
  )

  // Parse an optional, comma separated list of uints with explicit post-skip.
  // Note that we have to put the rule into parentheses here, because it 
  // contains an unparenthesized comma.
  BOOST_SPIRIT_RULE_PARSER(line,
    -,-,-,

    (
      ! uint_list[ (uint_action = & echo_uint), (next_action = & comma) ]
      >> lexeme_d[ !space_p ]
    )
  )

  bool parse_line(char const * str)
  {
    return BOOST_SPIRIT_CLASSIC_NS::parse(str,line,space_p).full;
  }

  #undef BOOST_SPIRIT__NAMESPACE

} } // namespace ::my_project::my_module


int main()
{
  std::string str;
  while (std::getline(std::cin, str))
  {
    if (str.empty())
      break;

    str += '\n';

    if (my_project::my_module::parse_line(str.c_str()))
      std::cout << "\nOK." << std::endl;
    else
      std::cout << "\nERROR." << std::endl;
  }
  return 0;
}

