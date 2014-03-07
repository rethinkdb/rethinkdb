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
#include "verify_return.hpp"

// This should verify that _1 etc are lvalues, but we leave that
// to the "tricky" test as the Boost implementation doesn't always
// guarentee that:
template <class T>
void check_placeholder(T)
{
   T t; 
   T t2(t);
   (void)t2;
   //BOOST_STATIC_ASSERT(::std::tr1::is_placeholder<T>::value);
}

template <class Binder, class R>
void check_bind0(Binder b, R r)
{
   //BOOST_STATIC_ASSERT(::std::tr1::is_bind_expression<Binder>::value);
   verify_return_type(b(), r);

   typedef typename Binder::result_type result_type;
   BOOST_STATIC_ASSERT((::boost::is_same<result_type, R>::value));
}

template <class Binder, class R, class A1>
void check_bind1(Binder b, R r, A1 a1)
{
   //BOOST_STATIC_ASSERT(::std::tr1::is_bind_expression<Binder>::value);
   verify_return_type(b(a1), r);
}

double test_proc(int a, char b, long c)
{
   return a+b+c;
}

struct from_double
{
   from_double(double){}
};


int main()
{
   check_placeholder(std::tr1::placeholders::_1);
   check_placeholder(std::tr1::placeholders::_2);
   check_placeholder(std::tr1::placeholders::_3);
   check_placeholder(std::tr1::placeholders::_4);
   check_placeholder(std::tr1::placeholders::_5);
   check_placeholder(std::tr1::placeholders::_6);
   check_placeholder(std::tr1::placeholders::_7);
   check_placeholder(std::tr1::placeholders::_8);
   check_placeholder(std::tr1::placeholders::_9);

   check_bind0(std::tr1::bind(&test_proc, 0, 0, 0), double(0));
   //check_bind0(std::tr1::bind<double>(&test_proc, 0, 0, 0), double(0));
   check_bind1(std::tr1::bind(&test_proc, 0, 0, std::tr1::placeholders::_1), double(0), 0);
   check_bind1(std::tr1::bind(&test_proc, std::tr1::placeholders::_1, 0, 0), double(0), 0);
   
   check_bind0(std::tr1::bind(test_proc, 0, 0, 0), double(0));
   //check_bind0(std::tr1::bind<double>(test_proc, 0, 0, 0), double(0));
   check_bind1(std::tr1::bind(test_proc, 0, 0, std::tr1::placeholders::_1), double(0), 0);
   check_bind1(std::tr1::bind(test_proc, std::tr1::placeholders::_1, 0, 0), double(0), 0);

   check_bind0(std::tr1::bind(std::plus<double>(), 0, 1), double(0));
   check_bind0(std::tr1::bind<double>(std::plus<double>(), 1, 0), double(0));
   check_bind0(std::tr1::bind<from_double>(std::plus<double>(), 1, 0), from_double(0));
   check_bind1(std::tr1::bind(std::plus<double>(), 0, std::tr1::placeholders::_1), double(0), 0);
   check_bind1(std::tr1::bind(std::plus<double>(), std::tr1::placeholders::_1, 0), double(0), 0);
   return 0;
}

