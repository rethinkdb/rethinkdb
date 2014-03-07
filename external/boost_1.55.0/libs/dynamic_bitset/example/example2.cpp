// (C) Copyright Jeremy Siek 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  Sample output:
//
//  bits(0) = 00
//  bits(1) = 01
//  bits(2) = 10
//  bits(3) = 11


#include <iostream>
#include <boost/dynamic_bitset.hpp>

int main()
{
  const boost::dynamic_bitset<> b0(2, 0ul);
  std::cout << "bits(0) = " << b0 << std::endl;

  const boost::dynamic_bitset<> b1(2, 1ul);
  std::cout << "bits(1) = " << b1 << std::endl;

  const boost::dynamic_bitset<> b2(2, 2ul);
  std::cout << "bits(2) = " << b2 << std::endl;

  const boost::dynamic_bitset<> b3(2, 3ul);
  std::cout << "bits(3) = " << b3 << std::endl;

  return 0;
}
