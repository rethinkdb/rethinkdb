//[foo_cpp_copyright
/*=============================================================================
    Copyright (c) 2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
//]

//[foo_cpp
struct Foo{

  Foo()//=;
//<-
      : x( 10 )
  {}
//->

//<-
  int x;
//->
};

/*=
int main()
{
    Foo x;
}
*/
//]
