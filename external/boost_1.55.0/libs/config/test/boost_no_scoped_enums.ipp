//  (C) Copyright Beman Dawes 2008

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_SCOPED_ENUMS
//  TITLE:         C++0x scoped enum unavailable
//  DESCRIPTION:   The compiler does not support C++0x scoped enum

namespace boost_no_cxx11_scoped_enums {

int test()
{
  enum class scoped_enum { yes, no, maybe };
  // This tests bug http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38064
  bool b = (scoped_enum::yes == scoped_enum::yes) 
   && (scoped_enum::yes != scoped_enum::no) 
   && (scoped_enum::yes < scoped_enum::no) 
   && (scoped_enum::yes <= scoped_enum::no) 
   && (scoped_enum::no > scoped_enum::yes) 
   && (scoped_enum::no >= scoped_enum::yes);
  return b ? 0 : 1;
}

}
