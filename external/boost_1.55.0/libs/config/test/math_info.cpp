//  (C) Copyright John Maddock 2005. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config/test for most recent version.

//
// This test prints out informative information about <math.h>, <float.h>
// and <limits>.  Note that this file does require a correctly configured
// Boost setup, and so can't be folded into config_info which is designed
// to function without Boost.Confg support.  Each test is documented in
// more detail below.
//

#include <boost/limits.hpp>
#include <limits.h>
#include <math.h>
#include <cmath>
#include <float.h>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <boost/type_traits/alignment_of.hpp>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{ using ::strcmp; using ::pow; using ::fabs; using ::sqrt; using ::sin; using ::atan2; }
#endif

static unsigned int indent = 4;
static unsigned int width = 40;

void print_macro(const char* name, const char* value)
{
   // if name == value+1 then then macro is not defined,
   // in which case we don't print anything:
   if(0 != std::strcmp(name, value+1))
   {
      for(unsigned i = 0; i < indent; ++i) std::cout.put(' ');
      std::cout << std::setw(width);
      std::cout.setf(std::istream::left, std::istream::adjustfield);
      std::cout << name;
      if(value[1])
      {
         // macro has a value:
         std::cout << value << "\n";
      }
      else
      {
         // macro is defined but has no value:
         std::cout << " [no value]\n";
      }
   }
}

#define PRINT_MACRO(X) print_macro(#X, BOOST_STRINGIZE(=X))

template <class T>
void print_expression(const char* expression, T val)
{
   for(unsigned i = 0; i < indent; ++i) std::cout.put(' ');
   std::cout << std::setw(width);
   std::cout.setf(std::istream::left, std::istream::adjustfield);
   std::cout << std::setprecision(std::numeric_limits<T>::digits10+2);
   std::cout << expression << "=" << val << std::endl;
}

#define PRINT_EXPRESSION(E) print_expression(#E, E);


template <class T>
void print_limits(T, const char* name)
{
   //
   // Output general information on numeric_limits, as well as 
   // probing known and supected problems.
   //
   std::cout << 
      "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
      "std::numeric_limits information for type " << name << std::endl;
   std::cout << 
      "    is_specialized       = " << std::numeric_limits<T>::is_specialized << std::endl;
   std::cout << 
      "    min" "()                = " << std::setprecision(std::numeric_limits<T>::digits10 + 2) << (std::numeric_limits<T>::min)() << std::endl;
   std::cout << 
      "    max" "()                = " << std::setprecision(std::numeric_limits<T>::digits10 + 2) << (std::numeric_limits<T>::max)() << std::endl;
   std::cout << 
      "    digits               = " << std::numeric_limits<T>::digits << std::endl;
   std::cout << 
      "    digits10             = " << std::numeric_limits<T>::digits10 << std::endl;
   std::cout << 
      "    is_signed            = " << std::numeric_limits<T>::is_signed << std::endl;
   std::cout << 
      "    is_integer           = " << std::numeric_limits<T>::is_integer << std::endl;
   std::cout << 
      "    is_exact             = " << std::numeric_limits<T>::is_exact << std::endl;
   std::cout << 
      "    radix                = " << std::numeric_limits<T>::radix << std::endl;

   std::cout << 
      "    epsilon()            = " << std::setprecision(std::numeric_limits<T>::digits10 + 2) << (std::numeric_limits<T>::epsilon)() << std::endl;
   std::cout << 
      "    round_error()        = " << std::setprecision(std::numeric_limits<T>::digits10 + 2) << (std::numeric_limits<T>::round_error)() << std::endl;

   std::cout << 
      "    min_exponent         = " << std::numeric_limits<T>::min_exponent << std::endl;
   std::cout << 
      "    min_exponent10       = " << std::numeric_limits<T>::min_exponent10 << std::endl;
   std::cout << 
      "    max_exponent         = " << std::numeric_limits<T>::max_exponent << std::endl;
   std::cout << 
      "    max_exponent10       = " << std::numeric_limits<T>::max_exponent10 << std::endl;
   std::cout << 
      "    has_infinity         = " << std::numeric_limits<T>::has_infinity << std::endl;
   std::cout << 
      "    has_quiet_NaN        = " << std::numeric_limits<T>::has_quiet_NaN << std::endl;
   std::cout << 
      "    has_signaling_NaN    = " << std::numeric_limits<T>::has_signaling_NaN << std::endl;
   std::cout << 
      "    has_denorm           = " << std::numeric_limits<T>::has_denorm << std::endl;
   std::cout << 
      "    has_denorm_loss      = " << std::numeric_limits<T>::has_denorm_loss << std::endl;

   std::cout << 
      "    infinity()           = " << std::setprecision(std::numeric_limits<T>::digits10 + 2) << (std::numeric_limits<T>::infinity)() << std::endl;
   std::cout << 
      "    quiet_NaN()          = " << std::setprecision(std::numeric_limits<T>::digits10 + 2) << (std::numeric_limits<T>::quiet_NaN)() << std::endl;
   std::cout << 
      "    signaling_NaN()      = " << std::setprecision(std::numeric_limits<T>::digits10 + 2) << (std::numeric_limits<T>::signaling_NaN)() << std::endl;
   std::cout << 
      "    denorm_min()         = " << std::setprecision(std::numeric_limits<T>::digits10 + 2) << (std::numeric_limits<T>::denorm_min)() << std::endl;
   

   std::cout << 
      "    is_iec559            = " << std::numeric_limits<T>::is_iec559 << std::endl;
   std::cout << 
      "    is_bounded           = " << std::numeric_limits<T>::is_bounded << std::endl;
   std::cout << 
      "    is_modulo            = " << std::numeric_limits<T>::is_modulo << std::endl;
   std::cout << 
      "    traps                = " << std::numeric_limits<T>::traps << std::endl;
   std::cout << 
      "    tinyness_before      = " << std::numeric_limits<T>::tinyness_before << std::endl;
   std::cout << 
      "    round_style          = " << std::numeric_limits<T>::round_style << std::endl << std::endl;

   if(std::numeric_limits<T>::is_exact == 0)
   {
      bool r = std::numeric_limits<T>::epsilon() == std::pow(static_cast<T>(std::numeric_limits<T>::radix), 1-std::numeric_limits<T>::digits);
      if(r)
         std::cout << "Epsilon has sane value of std::pow(std::numeric_limits<T>::radix, 1-std::numeric_limits<T>::digits)." << std::endl;
      else
         std::cout << "CAUTION: epsilon does not have a sane value." << std::endl;
      std::cout << std::endl;
   }
   std::cout << 
      "    sizeof(" << name << ") = " << sizeof(T) << std::endl;
   std::cout << 
      "    alignment_of<" << name << "> = " << boost::alignment_of<T>::value << std::endl << std::endl;
}
/*
template <class T>
bool is_same_type(T, T)
{
   return true;
}*/
bool is_same_type(float, float)
{ return true; }
bool is_same_type(double, double)
{ return true; }
bool is_same_type(long double, long double)
{ return true; }
template <class T, class U>
bool is_same_type(T, U)
{
   return false;
}

//
// We need this to test whether abs has been overloaded for
// the floating point types or not:
//
namespace std{
#if !BOOST_WORKAROUND(BOOST_MSVC, == 1300) && \
    !defined(_LIBCPP_VERSION)
template <class T>
char abs(T)
{
   return ' ';
}
#endif
}


template <class T>
void test_overloads(T, const char* name)
{
   //
   // Probe known and suspected problems with the std lib Math functions.
   //
   std::cout << 
      "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
      "Math function overload information for type " << name << std::endl;

   //
   // Are the math functions overloaded for type T,
   // or do we just get double versions?
   //
   bool r = is_same_type(std::fabs(T(0)), T(0));
   r &= is_same_type(std::sqrt(T(0)), T(0));
   r &= is_same_type(std::sin(T(0)), T(0));
   if(r)
      std::cout << "The Math functions are overloaded for type " << name << std::endl;
   else
      std::cout << "CAUTION: The Math functions are NOT overloaded for type " << name << std::endl;

   //
   // Check that a few of the functions work OK, we do this because if these
   // are implemented as double precision internally then we can get
   // overflow or underflow when passing arguments of other types.
   //
   r = (std::fabs((std::numeric_limits<T>::max)()) == (std::numeric_limits<T>::max)());
   r &= (std::fabs(-(std::numeric_limits<T>::max)()) == (std::numeric_limits<T>::max)());
   r &= (std::fabs((std::numeric_limits<T>::min)()) == (std::numeric_limits<T>::min)());
   r &= (std::fabs(-(std::numeric_limits<T>::min)()) == (std::numeric_limits<T>::min)());
   if(r)
      std::cout << "std::fabs looks OK for type " << name << std::endl;
   else
      std::cout << "CAUTION: std::fabs is broken for type " << name << std::endl;

   //
   // abs not overloaded for real arguments with VC6 (and others?)
   //
   r = (std::abs((std::numeric_limits<T>::max)()) == (std::numeric_limits<T>::max)());
   r &= (std::abs(-(std::numeric_limits<T>::max)()) == (std::numeric_limits<T>::max)());
   r &= (std::abs((std::numeric_limits<T>::min)()) == (std::numeric_limits<T>::min)());
   r &= (std::abs(-(std::numeric_limits<T>::min)()) == (std::numeric_limits<T>::min)());
   if(r)
      std::cout << "std::abs looks OK for type " << name << std::endl;
   else
      std::cout << "CAUTION: std::abs is broken for type " << name << std::endl;

   //
   // std::sqrt on FreeBSD converts long double arguments to double leading to
   // overflow/underflow:
   //
   r = (std::sqrt((std::numeric_limits<T>::max)()) < (std::numeric_limits<T>::max)());
   if(r)
      std::cout << "std::sqrt looks OK for type " << name << std::endl;
   else
      std::cout << "CAUTION: std::sqrt is broken for type " << name << std::endl;

   //
   // Sanity check for atan2: verify that it returns arguments in the correct
   // range and not just atan(x/y).
   //
   static const T half_pi = static_cast<T>(1.57079632679489661923132169163975144L);

   T val = std::atan2(T(-1), T(-1));
   r = -half_pi > val;
   val = std::atan2(T(1), T(-1));
   r &= half_pi < val;
   val = std::atan2(T(1), T(1));
   r &= (val > 0) && (val < half_pi);
   val = std::atan2(T(-1), T(1));
   r &= (val < 0) && (val > -half_pi);
   if(r)
      std::cout << "std::atan2 looks OK for type " << name << std::endl;
   else
      std::cout << "CAUTION: std::atan2 is broken for type " << name << std::endl;
}



int main()
{
   //
   // Start by printing the values of the macros from float.h
   //
   std::cout << 
      "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
      "Macros from <math.h>" << std::endl;

#ifdef __BORLANDC__
   // Turn off hardware exceptions so we don't just abort 
   // when calling numeric_limits members.
   _control87(MCW_EM,MCW_EM);
#endif

   PRINT_EXPRESSION(HUGE_VAL);
#ifdef HUGE_VALF
   PRINT_EXPRESSION(HUGE_VALF);
#endif
#ifdef HUGE_VALL
   PRINT_EXPRESSION(HUGE_VALL);
#endif
#ifdef INFINITY
   PRINT_EXPRESSION(INFINITY);
#endif

   PRINT_MACRO(NAN);
   PRINT_MACRO(FP_INFINITE);
   PRINT_MACRO(FP_NAN);
   PRINT_MACRO(FP_NORMAL);
   PRINT_MACRO(FP_SUBNORMAL);
   PRINT_MACRO(FP_ZERO);
   PRINT_MACRO(FP_FAST_FMA);
   PRINT_MACRO(FP_FAST_FMAF);
   PRINT_MACRO(FP_FAST_FMAL);
   PRINT_MACRO(FP_ILOGB0);
   PRINT_MACRO(FP_ILOGBNAN);
   PRINT_MACRO(MATH_ERRNO);
   PRINT_MACRO(MATH_ERREXCEPT);

   PRINT_EXPRESSION(FLT_MIN_10_EXP);
   PRINT_EXPRESSION(FLT_DIG);
   PRINT_EXPRESSION(FLT_MIN_EXP);
   PRINT_EXPRESSION(FLT_EPSILON);
   PRINT_EXPRESSION(FLT_RADIX);
   PRINT_EXPRESSION(FLT_MANT_DIG);
   PRINT_EXPRESSION(FLT_ROUNDS);
   PRINT_EXPRESSION(FLT_MAX);
   PRINT_EXPRESSION(FLT_MAX_10_EXP);
   PRINT_EXPRESSION(FLT_MAX_EXP);
   PRINT_EXPRESSION(FLT_MIN);
   PRINT_EXPRESSION(DBL_DIG);
   PRINT_EXPRESSION(DBL_MIN_EXP);
   PRINT_EXPRESSION(DBL_EPSILON);
   PRINT_EXPRESSION(DBL_MANT_DIG);
   PRINT_EXPRESSION(DBL_MAX);
   PRINT_EXPRESSION(DBL_MIN);
   PRINT_EXPRESSION(DBL_MAX_10_EXP);
   PRINT_EXPRESSION(DBL_MAX_EXP);
   PRINT_EXPRESSION(DBL_MIN_10_EXP);
   PRINT_EXPRESSION(LDBL_MAX_10_EXP);
   PRINT_EXPRESSION(LDBL_MAX_EXP);
   PRINT_EXPRESSION(LDBL_MIN);
   PRINT_EXPRESSION(LDBL_MIN_10_EXP);
   PRINT_EXPRESSION(LDBL_DIG);
   PRINT_EXPRESSION(LDBL_MIN_EXP);
   PRINT_EXPRESSION(LDBL_EPSILON);
   PRINT_EXPRESSION(LDBL_MANT_DIG);
   PRINT_EXPRESSION(LDBL_MAX);

   std::cout << std::endl;

   //
   // print out numeric_limits info:
   //
   print_limits(float(0), "float");
   print_limits(double(0), "double");
   print_limits((long double)(0), "long double");

   //
   // print out function overload information:
   //
   test_overloads(float(0), "float");
   test_overloads(double(0), "double");
   test_overloads((long double)(0), "long double");
   return 0;
}




