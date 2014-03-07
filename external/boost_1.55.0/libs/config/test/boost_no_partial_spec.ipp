//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
//  TITLE:         partial specialisation
//  DESCRIPTION:   Class template partial specialization
//                 (14.5.4 [temp.class.spec]) not supported.


namespace boost_no_template_partial_specialization{

template <class T>
struct partial1
{
   typedef T& type;
};

template <class T>
struct partial1<T&>
{
   typedef T& type;
};

template <class T, bool b>
struct partial2
{
   typedef T& type;
};

template <class T>
struct partial2<T,true>
{
   typedef T type;
};


int test()
{
   int i = 0;
   partial1<int&>::type p1 = i;
   partial2<int&,true>::type p2 = i;
   (void)p1;
   (void)p2;
   (void)i;
   return 0;
}

}




