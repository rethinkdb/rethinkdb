//  (C) Copyright John Maddock 2008. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_PARTIAL_SPECIALIZATION_IMPLICIT_DEFAULT_ARGS
//  TITLE:         Default arguments in partial specialization
//  DESCRIPTION:   The compiler chokes if a partial specialization relies on default arguments in the primary template.

namespace boost_no_partial_specialization_implicit_default_args{

template <class T>
struct one
{
};

template <class T1, class T2 = void>
struct tag
{
};

template <class T1>
struct tag<one<T1> >
{
};

template <class T>
void consume_variable(T const&){}

int test()
{
   tag<int> t1;
   consume_variable(t1);
   tag<one<int> > t2;
   consume_variable(t2);
   tag<int, double> t3;
   consume_variable(t3);
   tag<one<int>, double> t4;
   consume_variable(t4);
   return 0;
}

}

