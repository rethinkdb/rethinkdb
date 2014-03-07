// Copyright (C) 2004 Jeremy Siek <jsiek@cs.indiana.edu>
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/iterator/reverse_iterator.hpp>
#include <boost/cstdlib.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>

int main()
{
  int x[] = { 1, 2, 3, 4 };
  boost::reverse_iterator<int*> first(x + 4), last(x);
  std::copy(first, last, std::ostream_iterator<int>(std::cout, " "));
  std::cout << std::endl;
  return 0;
}
