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


namespace my_project { namespace my_module {

  using namespace BOOST_SPIRIT_CLASSIC_NS;


  // A semantic action.
  void echo_uint(unsigned i) { std::cout << "- " << i << std::endl; }


  #define BOOST_SPIRIT__NAMESPACE (2,(my_project,my_module))

  // C/C++ comment and whitespace skip parser..
  BOOST_SPIRIT_RULE_PARSER(skipper,
    -,-,-, 

    (   confix_p("//",*anychar_p,eol_p) 
      | confix_p("/*",*anychar_p,"*/") 
      | space_p 
    )
  )

  // Parser for unsigned decimal, hexadecimal and binary literals.
  BOOST_SPIRIT_RULE_PARSER(uint_literal,
    -,-,-,

      "0x" >> hex_p[ & echo_uint ]
    | "0b" >> bin_p[ & echo_uint ]
    | uint_p[ & echo_uint ]
  ) 

  // A generic list parser (in some ways similar to Spirit's list_p utility or
  // the % operator) with two parameters.
  BOOST_SPIRIT_RULE_PARSER(enumeration_parser,
    (2,( element_parser, delimiter_parser )),-,-,

    element_parser >> *(delimiter_parser >> element_parser)
  ) 

  // Parse an optional, comma separated list of uints with explicit post-skip.
  BOOST_SPIRIT_RULE_PARSER(line,
    -,-,-,
    ! enumeration_parser(uint_literal,',') 
    >> lexeme_d[ !skipper ]
  )

  bool parse_line(char const * str)
  {
    return BOOST_SPIRIT_CLASSIC_NS::parse(str,line,skipper).full;
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

