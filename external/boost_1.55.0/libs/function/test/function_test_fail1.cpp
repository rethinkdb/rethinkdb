// Boost.Function library

//  Copyright (C) Douglas Gregor 2001-2005. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/test/minimal.hpp>
#include <boost/function.hpp>

using namespace std;
using namespace boost;

int
test_main(int, char*[])
{
  function0<int> f1;
  function0<int> f2;

  if (f1 == f2) {
  }

  BOOST_ERROR("This should not have compiled.");

  return 0;
}
