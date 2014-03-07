//  Copyright (C) 2011 Takaya Saito
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_CXX11_NOEXCEPT
//  TITLE:         C++0x noexcept unavailable
//  DESCRIPTION:   The compiler does not support C++0x noexcept

namespace boost_no_cxx11_noexcept {

void quiet_warning(bool){}

int f() noexcept ;
int g() noexcept( noexcept( f() ) ) ;

int test()
{
  bool b = noexcept( g() );
  quiet_warning(b);
  return 0;
}

}
