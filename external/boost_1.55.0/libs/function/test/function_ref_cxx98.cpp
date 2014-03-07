// Function library

// Copyright (C) 2001-2003 Douglas Gregor

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt) 

// For more information, see http://www.boost.org/


#include <boost/function.hpp>
#include <iostream>


struct stateful_type { int operator()(int) const { return 0; } };

int main()
{
    stateful_type a_function_object;
  boost::function<int (int)> f;
  f = boost::ref(a_function_object);

  boost::function<int (int)> f2(f);

    return 0;
}
