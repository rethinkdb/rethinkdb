//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_VOID_RETURNS
//  TITLE:         no void returns
//  DESCRIPTION:   The compiler does not allow a void function 
//                 to return the result of calling another void
//                 function.
//  
//                 void f() {}
//                 void g() { return f(); }


namespace boost_no_void_returns{

void f() {}

void g() { return f(); }

int test()
{
    return 0;
}

}





