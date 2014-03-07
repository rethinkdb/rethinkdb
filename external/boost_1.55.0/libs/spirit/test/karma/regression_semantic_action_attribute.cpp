//  Copyright (c) 2010 Michael Caisse
//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <string>
#include <vector>
#include <boost/spirit/include/karma.hpp> 
#include <boost/spirit/include/phoenix.hpp>

#include "test.hpp"

using namespace spirit_test;

namespace karma = boost::spirit::karma;
namespace phx   = boost::phoenix;

int main()
{
  using karma::int_;
  using karma::_1;

  BOOST_TEST(test("16909060", int_[ _1 = phx::val(0x01020304) ]));

  // make sure the passed attribute type does not enforce the attribute type 
  // for the semantic action
  unsigned char char_value = 8;
  BOOST_TEST(test("16909060", int_[ _1 = phx::val(0x01020304) ], char_value));

  return boost::report_errors();
}
