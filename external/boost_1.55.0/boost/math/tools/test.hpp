//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_TEST_HPP
#define BOOST_MATH_TOOLS_TEST_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/config.hpp>
#include <boost/math/tools/stats.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/test/test_tools.hpp>
#include <stdexcept>
#include <iostream>
#include <iomanip>

namespace boost{ namespace math{ namespace tools{

template <class T>
struct test_result
{
private:
   boost::math::tools::stats<T> stat;   // Statistics for the test.
   unsigned worst_case;                 // Index of the worst case test.
public:
   test_result() { worst_case = 0; }
   void set_worst(int i){ worst_case = i; }
   void add(const T& point){ stat.add(point); }
   // accessors:
   unsigned worst()const{ return worst_case; }
   T min BOOST_PREVENT_MACRO_SUBSTITUTION()const{ return (stat.min)(); }
   T max BOOST_PREVENT_MACRO_SUBSTITUTION()const{ return (stat.max)(); }
   T total()const{ return stat.total(); }
   T mean()const{ return stat.mean(); }
   boost::uintmax_t count()const{ return stat.count(); }
   T variance()const{ return stat.variance(); }
   T variance1()const{ return stat.variance1(); }
   T rms()const{ return stat.rms(); }

   test_result& operator+=(const test_result& t)
   {
      if((t.stat.max)() > (stat.max)())
         worst_case = t.worst_case;
      stat += t.stat;
      return *this;
   }
};

template <class T>
struct calculate_result_type
{
   typedef typename T::value_type row_type;
   typedef typename row_type::value_type value_type;
};

template <class T>
T relative_error(T a, T b)
{
   BOOST_MATH_STD_USING
#ifdef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   //
   // If math.h has no long double support we can't rely
   // on the math functions generating exponents outside
   // the range of a double:
   //
   T min_val = (std::max)(
      tools::min_value<T>(),
      static_cast<T>((std::numeric_limits<double>::min)()));
   T max_val = (std::min)(
      tools::max_value<T>(),
      static_cast<T>((std::numeric_limits<double>::max)()));
#else
   T min_val = tools::min_value<T>();
   T max_val = tools::max_value<T>();
#endif

   if((a != 0) && (b != 0))
   {
      // TODO: use isfinite:
      if(fabs(b) >= max_val)
      {
         if(fabs(a) >= max_val)
            return 0;  // one infinity is as good as another!
      }
      // If the result is denormalised, treat all denorms as equivalent:
      if((a < min_val) && (a > 0))
         a = min_val;
      else if((a > -min_val) && (a < 0))
         a = -min_val;
      if((b < min_val) && (b > 0))
         b = min_val;
      else if((b > -min_val) && (b < 0))
         b = -min_val;
      return (std::max)(fabs((a-b)/a), fabs((a-b)/b));
   }

   // Handle special case where one or both are zero:
   if(min_val == 0)
      return fabs(a-b);
   if(fabs(a) < min_val)
      a = min_val;
   if(fabs(b) < min_val)
      b = min_val;
   return (std::max)(fabs((a-b)/a), fabs((a-b)/b));
}

#if defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
template <>
inline double relative_error<double>(double a, double b)
{
   BOOST_MATH_STD_USING
   //
   // On Mac OS X we evaluate "double" functions at "long double" precision,
   // but "long double" actually has a very slightly narrower range than "double"!  
   // Therefore use the range of "long double" as our limits since results outside
   // that range may have been truncated to 0 or INF:
   //
   double min_val = (std::max)((double)tools::min_value<long double>(), tools::min_value<double>());
   double max_val = (std::min)((double)tools::max_value<long double>(), tools::max_value<double>());

   if((a != 0) && (b != 0))
   {
      // TODO: use isfinite:
      if(b > max_val)
      {
         if(a > max_val)
            return 0;  // one infinity is as good as another!
      }
      // If the result is denormalised, treat all denorms as equivalent:
      if((a < min_val) && (a > 0))
         a = min_val;
      else if((a > -min_val) && (a < 0))
         a = -min_val;
      if((b < min_val) && (b > 0))
         b = min_val;
      else if((b > -min_val) && (b < 0))
         b = -min_val;
      return (std::max)(fabs((a-b)/a), fabs((a-b)/b));
   }

   // Handle special case where one or both are zero:
   if(min_val == 0)
      return fabs(a-b);
   if(fabs(a) < min_val)
      a = min_val;
   if(fabs(b) < min_val)
      b = min_val;
   return (std::max)(fabs((a-b)/a), fabs((a-b)/b));
}
#endif

template <class T>
void set_output_precision(T)
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif
   if(std::numeric_limits<T>::digits10)
   {
      std::cout << std::setprecision(std::numeric_limits<T>::digits10 + 2);
   }
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <class Seq>
void print_row(const Seq& row)
{
   set_output_precision(row[0]);
   for(unsigned i = 0; i < row.size(); ++i)
   {
      if(i)
         std::cout << ", ";
      std::cout << row[i];
   }
   std::cout << std::endl;
}

//
// Function test accepts an matrix of input values (probably a 2D boost::array)
// and calls two functors for each row in the array - one calculates a value
// to test, and one extracts the expected value from the array (or possibly
// calculates it at high precision).  The two functors are usually simple lambda
// expressions.
//
template <class A, class F1, class F2>
test_result<typename calculate_result_type<A>::value_type> test(const A& a, F1 test_func, F2 expect_func)
{
   typedef typename A::value_type         row_type;
   typedef typename row_type::value_type  value_type;

   test_result<value_type> result;

   for(unsigned i = 0; i < a.size(); ++i)
   {
      const row_type& row = a[i];
      value_type point;
      try
      {
         point = test_func(row);
      }
      catch(const std::underflow_error&)
      {
         point = 0;
      }
      catch(const std::overflow_error&)
      {
         point = std::numeric_limits<value_type>::has_infinity ? 
            std::numeric_limits<value_type>::infinity()
            : tools::max_value<value_type>();
      }
      catch(const std::exception& e)
      {
         std::cerr << e.what() << std::endl;
         print_row(row);
         BOOST_ERROR("Unexpected exception.");
         // so we don't get further errors:
         point = expect_func(row);
      }
      value_type expected = expect_func(row);
      value_type err = relative_error(point, expected);
#ifdef BOOST_INSTRUMENT
      if(err != 0)
      {
         std::cout << row[0] << " " << err;
         if(std::numeric_limits<value_type>::is_specialized)
         {
            std::cout << " (" << err / std::numeric_limits<value_type>::epsilon() << "eps)";
         }
         std::cout << std::endl;
      }
#endif
      if(!(boost::math::isfinite)(point) && (boost::math::isfinite)(expected))
      {
         std::cout << "CAUTION: Found non-finite result, when a finite value was expected at entry " << i << "\n";
         std::cout << "Found: " << point << " Expected " << expected << " Error: " << err << std::endl;
         print_row(row);
         BOOST_ERROR("Unexpected non-finite result");
      }
      if(err > 0.5)
      {
         std::cout << "CAUTION: Gross error found at entry " << i << ".\n";
         std::cout << "Found: " << point << " Expected " << expected << " Error: " << err << std::endl;
         print_row(row);
         BOOST_ERROR("Gross error");
      }
      result.add(err);
      if((result.max)() == err)
         result.set_worst(i);
   }
   return result;
}

template <class Real, class A, class F1, class F2>
test_result<Real> test_hetero(const A& a, F1 test_func, F2 expect_func)
{
   typedef typename A::value_type         row_type;
   typedef Real                          value_type;

   test_result<value_type> result;

   for(unsigned i = 0; i < a.size(); ++i)
   {
      const row_type& row = a[i];
      value_type point;
      try
      {
         point = test_func(row);
      }
      catch(const std::underflow_error&)
      {
         point = 0;
      }
      catch(const std::overflow_error&)
      {
         point = std::numeric_limits<value_type>::has_infinity ? 
            std::numeric_limits<value_type>::infinity()
            : tools::max_value<value_type>();
      }
      catch(const std::exception& e)
      {
         std::cerr << e.what() << std::endl;
         print_row(row);
         BOOST_ERROR("Unexpected exception.");
         // so we don't get further errors:
         point = expect_func(row);
      }
      value_type expected = expect_func(row);
      value_type err = relative_error(point, expected);
#ifdef BOOST_INSTRUMENT
      if(err != 0)
      {
         std::cout << row[0] << " " << err;
         if(std::numeric_limits<value_type>::is_specialized)
         {
            std::cout << " (" << err / std::numeric_limits<value_type>::epsilon() << "eps)";
         }
         std::cout << std::endl;
      }
#endif
      if(!(boost::math::isfinite)(point) && (boost::math::isfinite)(expected))
      {
         std::cout << "CAUTION: Found non-finite result, when a finite value was expected at entry " << i << "\n";
         std::cout << "Found: " << point << " Expected " << expected << " Error: " << err << std::endl;
         print_row(row);
         BOOST_ERROR("Unexpected non-finite result");
      }
      if(err > 0.5)
      {
         std::cout << "CAUTION: Gross error found at entry " << i << ".\n";
         std::cout << "Found: " << point << " Expected " << expected << " Error: " << err << std::endl;
         print_row(row);
         BOOST_ERROR("Gross error");
      }
      result.add(err);
      if((result.max)() == err)
         result.set_worst(i);
   }
   return result;
}

} // namespace tools
} // namespace math
} // namespace boost

#endif


