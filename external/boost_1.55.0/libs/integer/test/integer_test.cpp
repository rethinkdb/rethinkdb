//  boost integer.hpp test program  ------------------------------------------//

//  Copyright Beman Dawes 1999.  
//  Copyright Daryle Walker 2001.  
//  Copyright John Maddock 2009.  
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


//  See http://www.boost.org/libs/integer for documentation.

//  Revision History
//   04 Oct 01  Added tests for new templates; rewrote code (Daryle Walker)
//   10 Mar 01  Boost Test Library now used for tests (Beman Dawes)
//   31 Aug 99  Initial version

#include <boost/detail/lightweight_test.hpp>  // for main, BOOST_TEST
#include <boost/integer.hpp>  // for boost::int_t, boost::uint_t
#include <boost/type_traits/is_same.hpp>

#include <climits>   // for ULONG_MAX, LONG_MAX, LONG_MIN
#include <iostream>  // for std::cout (std::endl indirectly)
#include <typeinfo>  // for std::type_info

#ifdef BOOST_MSVC
#pragma warning(disable:4127) // conditional expression is constant
#endif
#if defined( __BORLANDC__ )
# pragma option -w-8008 -w-8066 // condition is always true
#endif

//
// Keep track of error count, so we can print out detailed
// info only if we need it:
//
int last_error_count = 0;
//
// Helpers to print out the name of a type,
// we use these as typeid(X).name() doesn't always
// return a human readable string:
//
template <class T> const char* get_name_of_type(T){ return typeid(T).name(); }
const char* get_name_of_type(signed char){ return "signed char"; }
const char* get_name_of_type(unsigned char){ return "unsigned char"; }
const char* get_name_of_type(short){ return "short"; }
const char* get_name_of_type(unsigned short){ return "unsigned short"; }
const char* get_name_of_type(int){ return "int"; }
const char* get_name_of_type(unsigned int){ return "unsigned int"; }
const char* get_name_of_type(long){ return "long"; }
const char* get_name_of_type(unsigned long){ return "unsigned long"; }
#ifdef BOOST_HAS_LONG_LONG
const char* get_name_of_type(boost::long_long_type){ return "boost::long_long_type"; }
const char* get_name_of_type(boost::ulong_long_type){ return "boost::ulong_long_type"; }
#endif

template <int Bits>
void do_test_exact(boost::mpl::true_ const&)
{
   // Test the ::exact member:
   typedef typename boost::int_t<Bits>::exact int_exact;
   typedef typename boost::uint_t<Bits>::exact uint_exact;
   typedef typename boost::int_t<Bits>::least least_int;
   typedef typename boost::uint_t<Bits>::least least_uint;

   BOOST_TEST((boost::is_same<int_exact, least_int>::value));
   BOOST_TEST((boost::is_same<uint_exact, least_uint>::value));
}
template <int Bits>
void do_test_exact(boost::mpl::false_ const&)
{
   // Nothing to do, type does not have an ::extact member.
}

template <int Bits>
void do_test_bits()
{
   //
   // Recurse to next smallest number of bits:
   //
   do_test_bits<Bits - 1>();
   //
   // Test exact types if we have them:
   //
   do_test_exact<Bits>(
      boost::mpl::bool_<
      (sizeof(unsigned char) * CHAR_BIT == Bits)
      || (sizeof(unsigned short) * CHAR_BIT == Bits)
      || (sizeof(unsigned int) * CHAR_BIT == Bits)
      || (sizeof(unsigned long) * CHAR_BIT == Bits)
#ifdef BOOST_HAS_LONG_LONG
      || (sizeof(boost::ulong_long_type) * CHAR_BIT == Bits)
#endif
      >());
   //
   // We need to check all aspects of the members of int_t<Bits> and uint_t<Bits>:
   //
   typedef typename boost::int_t<Bits>::least least_int;
   typedef typename boost::int_t<Bits>::least fast_int;
   typedef typename boost::uint_t<Bits>::least least_uint;
   typedef typename boost::uint_t<Bits>::fast fast_uint;

   if(std::numeric_limits<least_int>::is_specialized)
   {
      BOOST_TEST(std::numeric_limits<least_int>::digits + 1 >= Bits);
   }
   if(std::numeric_limits<least_uint>::is_specialized)
   {
      BOOST_TEST(std::numeric_limits<least_uint>::digits >= Bits);
   }
   BOOST_TEST(sizeof(least_int) * CHAR_BIT >= Bits);
   BOOST_TEST(sizeof(least_uint) * CHAR_BIT >= Bits);
   BOOST_TEST(sizeof(fast_int) >= sizeof(least_int));
   BOOST_TEST(sizeof(fast_uint) >= sizeof(least_uint));
   //
   // There should be no type smaller than least_* that also has enough bits:
   //
   if(!boost::is_same<signed char, least_int>::value)
   {
      BOOST_TEST(std::numeric_limits<signed char>::digits < Bits);
      if(!boost::is_same<short, least_int>::value)
      {
         BOOST_TEST(std::numeric_limits<short>::digits < Bits);
         if(!boost::is_same<int, least_int>::value)
         {
            BOOST_TEST(std::numeric_limits<int>::digits < Bits);
            if(!boost::is_same<long, least_int>::value)
            {
               BOOST_TEST(std::numeric_limits<long>::digits < Bits);
            }
         }
      }
   }
   // And again, but unsigned:
   if(!boost::is_same<unsigned char, least_uint>::value)
   {
      BOOST_TEST(std::numeric_limits<unsigned char>::digits < Bits);
      if(!boost::is_same<unsigned short, least_uint>::value)
      {
         BOOST_TEST(std::numeric_limits<unsigned short>::digits < Bits);
         if(!boost::is_same<unsigned int, least_uint>::value)
         {
            BOOST_TEST(std::numeric_limits<unsigned int>::digits < Bits);
            if(!boost::is_same<unsigned long, least_uint>::value)
            {
               BOOST_TEST(std::numeric_limits<unsigned long>::digits < Bits);
            }
         }
      }
   }

   if(boost::detail::test_errors() != last_error_count)
   {
      last_error_count = boost::detail::test_errors();
      std::cout << "Errors occurred while testing with bit count = " << Bits << std::endl;
      std::cout << "Type int_t<" << Bits << ">::least was " << get_name_of_type(least_int(0)) << std::endl;
      std::cout << "Type int_t<" << Bits << ">::fast was " << get_name_of_type(fast_int(0)) << std::endl;
      std::cout << "Type uint_t<" << Bits << ">::least was " << get_name_of_type(least_uint(0)) << std::endl;
      std::cout << "Type uint_t<" << Bits << ">::fast was " << get_name_of_type(fast_uint(0)) << std::endl;
   }
}
template <>
void do_test_bits<-1>()
{
   // Nothing to do here!!
}

template <class Traits, class Expected>
void test_min_max_type(Expected val)
{
   typedef typename Traits::least least_type;
   typedef typename Traits::fast  fast_type;
   BOOST_TEST((boost::is_same<least_type, Expected>::value));
   BOOST_TEST(sizeof(fast_type) >= sizeof(least_type));
   BOOST_TEST((std::numeric_limits<least_type>::min)() <= val);
   BOOST_TEST((std::numeric_limits<least_type>::max)() >= val);

   if(boost::detail::test_errors() != last_error_count)
   {
      last_error_count = boost::detail::test_errors();
      std::cout << "Traits type is: " << typeid(Traits).name() << std::endl;
      std::cout << "Least type is: " << get_name_of_type(least_type(0)) << std::endl;
      std::cout << "Fast type is: " << get_name_of_type(fast_type(0)) << std::endl;
      std::cout << "Expected type is: " << get_name_of_type(Expected(0)) << std::endl;
      std::cout << "Required value is: " << val << std::endl;
   }
}

// Test program
int main(int, char*[])
{
   // Test int_t and unint_t first:
   if(std::numeric_limits<boost::intmax_t>::is_specialized)
      do_test_bits<std::numeric_limits<boost::uintmax_t>::digits>();
   else
      do_test_bits<std::numeric_limits<long>::digits>();

   //
   // Test min and max value types:
   //
   test_min_max_type<boost::int_max_value_t<SCHAR_MAX>, signed char>(SCHAR_MAX);
   test_min_max_type<boost::int_min_value_t<SCHAR_MIN>, signed char>(SCHAR_MIN);
   test_min_max_type<boost::uint_value_t<UCHAR_MAX>, unsigned char>(UCHAR_MAX);
#if(USHRT_MAX != UCHAR_MAX)
      test_min_max_type<boost::int_max_value_t<SCHAR_MAX+1>, short>(SCHAR_MAX+1);
      test_min_max_type<boost::int_min_value_t<SCHAR_MIN-1>, short>(SCHAR_MIN-1);
      test_min_max_type<boost::uint_value_t<UCHAR_MAX+1>, unsigned short>(UCHAR_MAX+1);
      test_min_max_type<boost::int_max_value_t<SHRT_MAX>, short>(SHRT_MAX);
      test_min_max_type<boost::int_min_value_t<SHRT_MIN>, short>(SHRT_MIN);
      test_min_max_type<boost::uint_value_t<USHRT_MAX>, unsigned short>(USHRT_MAX);
#endif
#if(UINT_MAX != USHRT_MAX)
      test_min_max_type<boost::int_max_value_t<SHRT_MAX+1>, int>(SHRT_MAX+1);
      test_min_max_type<boost::int_min_value_t<SHRT_MIN-1>, int>(SHRT_MIN-1);
      test_min_max_type<boost::uint_value_t<USHRT_MAX+1>, unsigned int>(USHRT_MAX+1);
      test_min_max_type<boost::int_max_value_t<INT_MAX>, int>(INT_MAX);
      test_min_max_type<boost::int_min_value_t<INT_MIN>, int>(INT_MIN);
      test_min_max_type<boost::uint_value_t<UINT_MAX>, unsigned int>(UINT_MAX);
#endif
#if(ULONG_MAX != UINT_MAX)
      test_min_max_type<boost::int_max_value_t<INT_MAX+1L>, long>(INT_MAX+1L);
      test_min_max_type<boost::int_min_value_t<INT_MIN-1L>, long>(INT_MIN-1L);
      test_min_max_type<boost::uint_value_t<UINT_MAX+1uL>, unsigned long>(UINT_MAX+1uL);
      test_min_max_type<boost::int_max_value_t<LONG_MAX>, long>(LONG_MAX);
      test_min_max_type<boost::int_min_value_t<LONG_MIN>, long>(LONG_MIN);
      test_min_max_type<boost::uint_value_t<ULONG_MAX>, unsigned long>(ULONG_MAX);
#endif
#ifndef BOOST_NO_INTEGRAL_INT64_T
#if defined(BOOST_HAS_LONG_LONG) && (defined(ULLONG_MAX) && (ULLONG_MAX != ULONG_MAX))
      test_min_max_type<boost::int_max_value_t<LONG_MAX+1LL>, boost::long_long_type>(LONG_MAX+1LL);
      test_min_max_type<boost::int_min_value_t<LONG_MIN-1LL>, boost::long_long_type>(LONG_MIN-1LL);
      test_min_max_type<boost::uint_value_t<ULONG_MAX+1uLL>, boost::ulong_long_type>(ULONG_MAX+1uLL);
      test_min_max_type<boost::int_max_value_t<LLONG_MAX>, boost::long_long_type>(LLONG_MAX);
      test_min_max_type<boost::int_min_value_t<LLONG_MIN>, boost::long_long_type>(LLONG_MIN);
      test_min_max_type<boost::uint_value_t<ULLONG_MAX>, boost::ulong_long_type>(ULLONG_MAX);
#endif
#if defined(BOOST_HAS_LONG_LONG) && (defined(ULONG_LONG_MAX) && (ULONG_LONG_MAX != ULONG_MAX))
      test_min_max_type<boost::int_max_value_t<LONG_MAX+1LL>, boost::long_long_type>(LONG_MAX+1LL);
      test_min_max_type<boost::int_min_value_t<LONG_MIN-1LL>, boost::long_long_type>(LONG_MIN-1LL);
      test_min_max_type<boost::uint_value_t<ULONG_MAX+1uLL>, boost::ulong_long_type>(ULONG_MAX+1uLL);
      test_min_max_type<boost::int_max_value_t<LONG_LONG_MAX>, boost::long_long_type>(LONG_LONG_MAX);
      test_min_max_type<boost::int_min_value_t<LONG_LONG_MIN>, boost::long_long_type>(LONG_LONG_MIN);
      test_min_max_type<boost::uint_value_t<ULONG_LONG_MAX>, boost::ulong_long_type>(ULONG_LONG_MAX);
#endif
#if defined(BOOST_HAS_LONG_LONG) && (defined(ULONGLONG_MAX) && (ULONGLONG_MAX != ULONG_MAX))
      test_min_max_type<boost::int_max_value_t<LONG_MAX+1LL>, boost::long_long_type>(LONG_MAX+1LL);
      test_min_max_type<boost::int_min_value_t<LONG_MIN-1LL>, boost::long_long_type>(LONG_MIN-1LL);
      test_min_max_type<boost::uint_value_t<ULONG_MAX+1uLL>, boost::ulong_long_type>(ULONG_MAX+1uLL);
      test_min_max_type<boost::int_max_value_t<LONGLONG_MAX>, boost::long_long_type>(LONGLONG_MAX);
      test_min_max_type<boost::int_min_value_t<LONGLONG_MIN>, boost::long_long_type>(LONGLONG_MAX);
      test_min_max_type<boost::uint_value_t<ULONGLONG_MAX>, boost::ulong_long_type>(ULONGLONG_MAX);
#endif
#if defined(BOOST_HAS_LONG_LONG) && (defined(_ULLONG_MAX) && defined(_LLONG_MIN) && (_ULLONG_MAX != ULONG_MAX))
      test_min_max_type<boost::int_max_value_t<LONG_MAX+1LL>, boost::long_long_type>(LONG_MAX+1LL);
      test_min_max_type<boost::int_min_value_t<LONG_MIN-1LL>, boost::long_long_type>(LONG_MIN-1LL);
      test_min_max_type<boost::uint_value_t<ULONG_MAX+1uLL>, boost::ulong_long_type>(ULONG_MAX+1uLL);
      test_min_max_type<boost::int_max_value_t<_LLONG_MAX>, boost::long_long_type>(_LLONG_MAX);
      test_min_max_type<boost::int_min_value_t<_LLONG_MIN>, boost::long_long_type>(_LLONG_MIN);
      test_min_max_type<boost::uint_value_t<_ULLONG_MAX>, boost::ulong_long_type>(_ULLONG_MAX);
#endif // BOOST_HAS_LONG_LONG
#endif // BOOST_NO_INTEGRAL_INT64_T
      return boost::report_errors();
}
