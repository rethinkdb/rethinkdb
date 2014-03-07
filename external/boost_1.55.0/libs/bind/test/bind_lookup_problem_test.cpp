//
// bind_lookup_problem_test.cpp
//
// Copyright (C) Markus Schoepflin 2005.
//
// Use, modification, and distribution are subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/bind.hpp>

template<class T> void value();

void f0() { }
void f1(int) { }
void f2(int, int) { }
void f3(int, int, int) { }
void f4(int, int, int, int) { }
void f5(int, int, int, int, int) { }
void f6(int, int, int, int, int, int) { }
void f7(int, int, int, int, int, int, int) { }
void f8(int, int, int, int, int, int, int, int) { }
void f9(int, int, int, int, int, int, int, int, int) { }

int main()
{
  boost::bind(f0);
  boost::bind(f1, 0);
  boost::bind(f2, 0, 0);
  boost::bind(f3, 0, 0, 0);
  boost::bind(f4, 0, 0, 0, 0);
  boost::bind(f5, 0, 0, 0, 0, 0);
  boost::bind(f6, 0, 0, 0, 0, 0, 0);
  boost::bind(f7, 0, 0, 0, 0, 0, 0, 0);
  boost::bind(f8, 0, 0, 0, 0, 0, 0, 0, 0);
  boost::bind(f9, 0, 0, 0, 0, 0, 0, 0, 0, 0);

  return 0;
}
