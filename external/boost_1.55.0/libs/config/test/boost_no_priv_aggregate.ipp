//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_PRIVATE_IN_AGGREGATE
//  TITLE:         private in aggregate types
//  DESCRIPTION:   The compiler misreads 8.5.1, treating classes
//                 as non-aggregate if they contain private or
//                 protected member functions.


namespace boost_no_private_in_aggregate{

struct t
{
private:
   void foo(){ i = j; }
public:
   void uncallable(); // silences warning from GCC
   int i;
   int j;
};


int test()
{
   t inst = { 0, 0, };
   (void) &inst;      // avoid "unused variable" warning
   return 0;
}

}




