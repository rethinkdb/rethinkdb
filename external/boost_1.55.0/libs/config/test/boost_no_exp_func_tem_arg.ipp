//  (C) Copyright John Maddock 2001.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_EXPLICIT_FUNCTION_TEMPLATE_ARGUMENTS
//  TITLE:         non-deduced function template parameters
//  DESCRIPTION:   Can only use deduced template arguments when
//                 calling function template instantiations.

#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1200)
#error "This is known to be buggy under VC6"
#endif


namespace boost_no_explicit_function_template_arguments{

struct foo
{
  template<class T> int bar(){return 0;}
  template<int I>   int bar(){return 1;}
};

int test_0()
{
  return 0;
}


template <int i>
bool foo_17041(int j)
{
   return (i == j);
}

int test()
{
   foo f;
   int a = f.bar<char>();
   int b = f.bar<2>();
   if((a !=0) || (b != 1))return -1;

   if(0 == foo_17041<8>(8)) return -1;
   if(0 == foo_17041<4>(4)) return -1;
   if(0 == foo_17041<5>(5)) return -1;
   return 0;
}

}




