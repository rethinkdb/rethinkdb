/* 
   Copyright (c) Marshall Clow 2011-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <boost/config.hpp>
#include <boost/algorithm/hex.hpp>

#include <string>
#include <iostream>
#include <vector>

//  should not compile: vector is not an integral type
int main( int , char* [] )
{
  std::vector<float> v;
  std::string out;
  boost::algorithm::unhex ( out, std::back_inserter(v));

  return 0;
}
