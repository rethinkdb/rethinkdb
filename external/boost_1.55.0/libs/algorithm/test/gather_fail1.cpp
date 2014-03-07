/* 
   Copyright (c) Marshall Clow 2011-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <iostream>

#include <boost/config.hpp>
#include <boost/algorithm/gather.hpp>

#include <string>
#include <vector>
#include <list>

#include "iterator_test.hpp"

namespace ba = boost::algorithm;

bool is_ten  ( int i ) { return i == 10; }

void test_sequence1 () {
    std::vector<int> v;
    typedef input_iterator<std::vector<int>::iterator> II;    

//  This should fail to compile, since gather doesn't work with input iterators
    (void) ba::gather ( II( v.begin ()), II( v.end ()), II( v.begin ()), is_ten );
    }


int main ()
{
  test_sequence1 ();
  return 0;
}
