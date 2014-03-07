// Copyright David Abrahams 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <vector>
#undef NDEBUG
#include "fake_sort.hpp"

int main()
{
  std::vector<int> v;
  fake::sort(v.begin(), v.end());
  return 0;
}
