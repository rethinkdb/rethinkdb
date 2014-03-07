// Copyright Daniel Wallin 2005. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter.hpp>
#include <string.h>
#include "basics.hpp"

namespace test
{
  template <class Params>
  void f(Params const &) {}
}

int main()
{
   using namespace test;

   f((name = 1, value = 1, test::index = 1, tester = 1,
      value = 1 // repeated keyword: should not compile
   ));
   return 0;
}

