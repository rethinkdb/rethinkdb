// Boost shared_ptr_example2_test main program  ------------------------------//

//  Copyright Beman Dawes 2001.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


//  See http://www.boost.org/libs/smart_ptr for documentation.

#include "shared_ptr_example2.hpp"

int main()
{
  example a;
  a.do_something();
  example b(a);
  b.do_something();
  example c;
  c = a;
  c.do_something();
  return 0;
}
