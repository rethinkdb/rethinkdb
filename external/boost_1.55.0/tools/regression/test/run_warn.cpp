//  (C) Copyright Beman Dawes 2003.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  Test naming convention: the portion of the name before the tilde ("~")
//  identifies the bjam test type. The portion after the tilde
//  identifies the correct result to be reported by compiler_status.

// provoke one or more compiler warnings

int main(int argc, char * argv[] )
{
  short s;
  unsigned long ul;
  s = s & ul; // warning from many compilers
  if ( s == ul ) {} // warning from GCC
  return 0;
}
