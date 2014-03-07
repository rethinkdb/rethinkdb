//  (C) Copyright Herve Bronnimann 2004.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <list>
#include <algorithm>
#include <cstdlib>
#include <cassert>
#include <iostream>

#include <boost/algorithm/minmax.hpp>
#include <boost/algorithm/minmax_element.hpp>

int main()
{
  using namespace std;

  // Demonstrating minmax()
  boost::tuple<int const&, int const&> result1 = boost::minmax(1, 0);
  assert( result1.get<0>() == 0 );
  assert( result1.get<1>() == 1 );


  // Demonstrating minmax_element()
  list<int> L;
  typedef list<int>::const_iterator iterator;
  generate_n(front_inserter(L), 1000, rand);
  pair< iterator, iterator > result2 = boost::minmax_element(L.begin(), L.end());

  cout << "The smallest element is " << *(result2.first) << endl;
  cout << "The largest element is  " << *(result2.second) << endl;

  assert( result2.first  == std::min_element(L.begin(), L.end()) );
  assert( result2.second == std::max_element(L.begin(), L.end()) );
}
