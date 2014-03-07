// Function library

// Copyright (C) 2001-2003 Douglas Gregor

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt) 

// For more information, see http://www.boost.org/

    
#include <boost/function.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>
#include <functional>

struct X {
  int foo(int);
  std::ostream& foo2(std::ostream&) const;
};
int X::foo(int x) { return -x; }
std::ostream& X::foo2(std::ostream& x) const { return x; }

int main()
{
    boost::function<int (X*, int)> f;
    boost::function<std::ostream& (X*, std::ostream&)> f2;

    f = &X::foo;
    f2 = &X::foo2;

    X x;
    BOOST_TEST(f(&x, 5) == -5);
    BOOST_TEST(f2(&x, boost::ref(std::cout)) == std::cout);

    return ::boost::report_errors();
}
