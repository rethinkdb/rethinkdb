//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef TEST_STD_HEADERS
#include <functional>
#else
#include <boost/tr1/functional.hpp>
#endif

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include "verify_return.hpp"

struct test_type
{
   int member();
   int cmember()const;
   int member2(char);
   int cmember2(char)const;
};

struct functor1 : public std::unary_function<int, double>
{
   double operator()(int)const;
};

struct functor2 : public std::binary_function<int, char, double>
{
   double operator()(int, char)const;
};

int main()
{
   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::unary_function<int, double>, std::tr1::reference_wrapper<double (int)> >::value));
   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::unary_function<int, double>, std::tr1::reference_wrapper<double (*)(int)> >::value));
   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::unary_function<test_type*, int>, std::tr1::reference_wrapper<int (test_type::*)()> >::value));
   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::unary_function<const test_type*, int>, std::tr1::reference_wrapper<int (test_type::*)()const> >::value));
   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::unary_function<int, double>, std::tr1::reference_wrapper<functor1> >::value));
   
   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::binary_function<int, char, double>, std::tr1::reference_wrapper<double (int, char)> >::value));
   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::binary_function<int, char, double>, std::tr1::reference_wrapper<double (*)(int, char)> >::value));
   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::binary_function<test_type*, char, int>, std::tr1::reference_wrapper<int (test_type::*)(char)> >::value));
   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::binary_function<const test_type*, char, int>, std::tr1::reference_wrapper<int (test_type::*)(char)const> >::value));
   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::binary_function<int, char, double>, std::tr1::reference_wrapper<functor2> >::value));

   test_type* ptt = 0;
   test_type const* cptt = 0;
   int zero = 0;

   // now check operator():
   std::tr1::reference_wrapper<double (int)>* pr1;
   verify_return_type((*pr1)(0), double());
   std::tr1::reference_wrapper<double (*)(int)>* pr2;
   verify_return_type((*pr2)(0), double());
   std::tr1::reference_wrapper<int (test_type::*)()>* pr3;
   verify_return_type((*pr3)(ptt), int());
   std::tr1::reference_wrapper<int (test_type::*)()const>* pr4;
   verify_return_type((*pr4)(cptt), int());
   std::tr1::reference_wrapper<functor1>* pr5;
   verify_return_type((*pr5)(zero), double());

   std::tr1::reference_wrapper<double (int, char)>* pr1b;
   verify_return_type((*pr1b)(0,0), double());
   std::tr1::reference_wrapper<double (*)(int, char)>* pr2b;
   verify_return_type((*pr2b)(0,0), double());
   std::tr1::reference_wrapper<int (test_type::*)(char)>* pr3b;
   verify_return_type((*pr3b)(ptt,zero), int());
   std::tr1::reference_wrapper<int (test_type::*)(char)const>* pr4b;
   verify_return_type((*pr4b)(cptt,zero), int());
   std::tr1::reference_wrapper<functor2>* pr5b;
   verify_return_type((*pr5b)(zero, zero), double());

   // check implicit convertion:
   int i = 0;
   int& ri = std::tr1::ref(i);
   const int& cri = std::tr1::cref(i);
}
