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
#include <boost/type_traits/is_base_and_derived.hpp>
#include "verify_return.hpp"

struct test_functor
{
   int operator()()const;
   long operator()(int)const;
   double operator()(int, char)const;
};

void nothing(){}

template <class Func>
void test_function(Func*)
{
   typedef typename Func::result_type result_type;
   
   test_functor t;
   Func f1;
   Func f2(0);
   Func f3(f1);
   Func f4(t);

   f2 = f1;
   f2 = 0;
   f2 = t;
   f2 = std::tr1::ref(t);
   f2 = std::tr1::cref(t);
   const Func& cf = f1;

   std::tr1::swap(f1, f2);
   if(cf) nothing();
   if(cf && f2) nothing();
   if(cf || f2) nothing();
   if(f2 && !cf) nothing();

   //const std::type_info& info = cf.target_type();
   test_functor* func1 = f1.template target<test_functor>();
   const test_functor* func2 = cf.template target<test_functor>();

   // comparison with null:
   if(0 == f1) nothing();
   if(0 != f1) nothing();
   if(f2 == 0) nothing();
   if(f2 != 0) nothing();
}


int main()
{
   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::exception, std::tr1::bad_function_call>::value));
   std::tr1::bad_function_call fe;

   test_function(static_cast<std::tr1::function<int (void)>* >(0));
   test_function(static_cast<std::tr1::function<long (int)>* >(0));
   test_function(static_cast<std::tr1::function<double (int,char)>* >(0));

   std::tr1::function<int (void)> f1;
   verify_return_type(f1(), int(0));
   std::tr1::function<long (int)> f2;
   verify_return_type(f2(0), long(0));
   std::tr1::function<double (int, char)> f3;
   verify_return_type(f3(0,0), double(0));

   //BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::unary_function<int, long>, std::tr1::function<long (int)> >::value));
   //BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::binary_function<int, char, double>, std::tr1::function<double (int,char)> >::value));
   return 0;
}

