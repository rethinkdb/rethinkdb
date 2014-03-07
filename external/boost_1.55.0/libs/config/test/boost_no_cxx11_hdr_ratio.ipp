//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_RATIO
//  TITLE:         C++0x header <ratio> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <ratio>

#include <ratio>

namespace boost_no_cxx11_hdr_ratio {

int test()
{
  using std::atto;
  using std::femto;
  using std::pico;
  using std::nano;
  using std::micro;
  using std::milli;
  using std::centi;
  using std::deci;
  using std::deca;
  using std::hecto;
  using std::kilo;
  using std::mega;
  using std::tera;
  using std::peta;
  using std::exa;
  return 0;
}

}
