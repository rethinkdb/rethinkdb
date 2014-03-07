// ------------------------------------------------------------------------------
// libs/format/test/format_test1.cpp :  test constructing objects and basic parsing
// ------------------------------------------------------------------------------

//  Copyright Samuel Krempp 2003. Use, modification, and distribution are
//  subject to the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/format for library home page

// ------------------------------------------------------------------------------

#include "boost/format.hpp"

#define BOOST_INCLUDE_MAIN 
#include <boost/test/test_tools.hpp>


int test_main(int, char* [])
{

  using boost::format;
  using boost::str;

  if(str( format("  %%  ") ) != "  %  ")
      BOOST_ERROR("Basic parsing without arguments Failed");
  if(str( format("nothing") ) != "nothing")
      BOOST_ERROR("Basic parsing without arguments Failed");
  if(str( format("%%  ") ) != "%  ")
      BOOST_ERROR("Basic parsing without arguments Failed");
  if(str( format("  %%") ) != "  %")
      BOOST_ERROR("Basic parsing without arguments Failed");
  if(str( format("  %n  ") ) != "    ")
      BOOST_ERROR("Basic parsing without arguments Failed");
  if(str( format("%n  ") ) != "  ")
      BOOST_ERROR("Basic parsing without arguments Failed");
  if(str( format("  %n") ) != "  ")
      BOOST_ERROR("Basic parsing without arguments Failed");

  if(str( format("%%##%%##%%1 %1%00") % "Escaped OK" ) != "%##%##%1 Escaped OK00")
      BOOST_ERROR("Basic parsing Failed");
  if(str( format("%%##%#x ##%%1 %s00") % 20 % "Escaped OK" ) != "%##0x14 ##%1 Escaped OK00")
      BOOST_ERROR("Basic p-parsing Failed") ;

  return 0;
}
