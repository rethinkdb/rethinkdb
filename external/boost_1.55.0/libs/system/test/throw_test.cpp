//  throw_test.cpp  --------------------------------------------------------===========-//

//  Copyright Beman Dawes 2010

//  Distributed under the Boost Software License, Version 1.0.
//  See www.boost.org/LICENSE_1_0.txt

//  Library home page is www.boost.org/libs/system

//--------------------------------------------------------------------------------------// 

//  See dynamic_link_test.cpp comments for use case.

//--------------------------------------------------------------------------------------// 

// define BOOST_SYSTEM_SOURCE so that <boost/system/config.hpp> knows
// the library is being built (possibly exporting rather than importing code)
#define BOOST_SYSTEM_SOURCE 

#include <boost/system/system_error.hpp>

namespace boost
{
  namespace system
  {
    BOOST_SYSTEM_DECL void throw_test()
    {
      throw system_error(9999, system_category(), "boo boo");
    }
  }
}
