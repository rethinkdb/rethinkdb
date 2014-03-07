/*==============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
//------------------------------------------------------------------------------
//  This example demonstrates the opaque rule parser.
//  See boost/spirit/include/rule_parser.hpp for details.
//------------------------------------------------------------------------------
#include <iostream>

#include <boost/typeof/typeof.hpp>

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_typeof.hpp>

#include <boost/spirit/include/classic_rule_parser.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

namespace my_project { namespace my_grammar {

  using namespace BOOST_SPIRIT_CLASSIC_NS;

  #define BOOST_SPIRIT__NAMESPACE (2,(my_project,my_grammar))

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(word,
    (1,( ((char const *),str) )), 
    -,

      lexeme_d[ str >> +space_p ]
  )

  BOOST_SPIRIT_OPAQUE_RULE_PARSER(main,
    -,-,

    *( word("dup") | word("swap") | word("drop") | word("rot") | word("tuck") )
  )

  #undef BOOST_SPIRIT__NAMESPACE

} } // namespace my_project::my_grammar



int main()
{
  std::string str;
  while (std::getline(std::cin, str))
  {
    if (str.empty())
      break;

    str += '\n';

    if (BOOST_SPIRIT_CLASSIC_NS::parse(str.c_str(), my_project::my_grammar::main).full)
      std::cout << "\nOK." << std::endl;
    else
      std::cout << "\nERROR." << std::endl;
  }
  return 0;
}


