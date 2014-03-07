//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_RANDOM
//  TITLE:         C++0x header <random> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <random>

#include <random>

namespace boost_no_cxx11_hdr_random {

int test()
{
  using std::minstd_rand0;
  using std::mt19937;
  using std::mt19937_64;
  using std::ranlux24_base;
  using std::ranlux48_base;
  using std::ranlux24;
  using std::ranlux48;
  using std::knuth_b;
  using std::default_random_engine;
  return 0;
}

}
