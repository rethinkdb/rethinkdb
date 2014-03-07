/* boost limits_test.cpp   test your <limits> file for important
 *
 * Copyright Jens Maurer 2000
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: limits_test.cpp 79537 2012-07-15 15:59:05Z marshall $
 */

#include <boost/limits.hpp>
#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>
#include <iostream>

/*
 * General portability note:
 * MSVC mis-compiles explicit function template instantiations.
 * For example, f<A>() and f<B>() are both compiled to call f<A>().
 * BCC is unable to implicitly convert a "const char *" to a std::string
 * when using explicit function template instantiations.
 *
 * Therefore, avoid explicit function template instantiations.
 */
#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
template<typename T> inline T make_char_numeric_for_streaming(T x) { return x; }
namespace fix{
inline int make_char_numeric_for_streaming(char c) { return c; }
inline int make_char_numeric_for_streaming(signed char c) { return c; }
inline int make_char_numeric_for_streaming(unsigned char c) { return c; }
}
using namespace fix;
#  if defined(_YVALS) && !defined(_CPPLIB_VER) && !defined(__SGI_STL_PORT) && !defined(_STLPORT_VERSION)
// fix for missing operator<< in original Dinkumware lib:
std::ostream& operator<<(std::ostream& os, __int64 i )
{
    char buf[80];
    sprintf(buf,"%I64d", i );
    os << buf;
    return os;
}
std::ostream& operator<<(std::ostream& os, unsigned __int64 i )
{
    char buf[80];
    sprintf(buf,"%I64u", i );
    os << buf;
    return os;
}
#  endif
#else
template<typename T> inline T make_char_numeric_for_streaming(T x) { return x; }
inline int make_char_numeric_for_streaming(char c) { return c; }
inline int make_char_numeric_for_streaming(signed char c) { return c; }
inline int make_char_numeric_for_streaming(unsigned char c) { return c; }
#endif

#if (defined(_GLIBCPP_VERSION) || defined(_GLIBCXX_VERSION)) \
   && defined(BOOST_HAS_LONG_LONG) \
   && !defined(_GLIBCPP_USE_LONG_LONG) \
   && !defined(_GLIBCXX_USE_LONG_LONG)
//
// Some libstdc++ versions have numeric_limits<long long> but no
// iostream support for long long.  TODO, find a better fix!!
//
std::ostream& operator<<(std::ostream& os, long long i )
{
    return os << static_cast<long double>(i);
}
std::ostream& operator<<(std::ostream& os, unsigned long long i )
{
    return os << static_cast<long double>(i);
}
#endif

template<class T>
void test_integral_limits(const T &, const char * msg)
{
  typedef std::numeric_limits<T> lim;
  std::cout << "Testing " << msg
            << " (size " << sizeof(T) << ")"
            << " min: " << make_char_numeric_for_streaming((lim::min)())
            << ", max: " << make_char_numeric_for_streaming((lim::max)())
            << std::endl;

  BOOST_CHECK(static_cast<bool>(lim::is_specialized));
  BOOST_CHECK(static_cast<bool>(lim::is_integer));
  // BOOST_CHECK(lim::is_modulo);
  BOOST_CHECK(static_cast<bool>((lim::min)() < (lim::max)()));
}

template <class T>
void print_hex_val(T t, const char* name)
{
  const unsigned char* p = (const unsigned char*)&t;
  std::cout << "hex value of " << name << " is: ";
  for (unsigned int i = 0; i < sizeof(T); ++i) {
    if(p[i] <= 0xF)
      std::cout << "0";
    std::cout << std::hex << (int)p[i];
  }
  std::cout << std::dec << std::endl;
}

template<class T>
void test_float_limits(const T &, const char * msg)
{
  std::cout << "\nTesting " << msg << std::endl;
  typedef std::numeric_limits<T> lim;

  BOOST_CHECK(static_cast<bool>(lim::is_specialized));
  BOOST_CHECK(static_cast<bool>(!lim::is_modulo));
  BOOST_CHECK(static_cast<bool>(!lim::is_integer));
  BOOST_CHECK(static_cast<bool>(lim::is_signed));

  const T infinity = lim::infinity();
  const T qnan = lim::quiet_NaN();
  const T snan = lim::signaling_NaN();

  std::cout << "IEEE-compatible: " << lim::is_iec559
       << ", traps: " << lim::traps
       << ", bounded: " << lim::is_bounded
       << ", exact: " << lim::is_exact << '\n'
       << "min: " << (lim::min)() << ", max: " << (lim::max)() << '\n'
       << "infinity: " << infinity << ", QNaN: " << qnan << '\n';
  print_hex_val((lim::max)(), "max");
  print_hex_val(infinity, "infinity");
  print_hex_val(qnan, "qnan");
  print_hex_val(snan, "snan");

  BOOST_CHECK((lim::max)() > 1000);
  BOOST_CHECK((lim::min)() > 0);
  BOOST_CHECK((lim::min)() < 0.001);
  BOOST_CHECK(lim::epsilon() > 0);

  if(lim::is_iec559) {
    BOOST_CHECK(static_cast<bool>(lim::has_infinity));
    BOOST_CHECK(static_cast<bool>(lim::has_quiet_NaN));
    BOOST_CHECK(static_cast<bool>(lim::has_signaling_NaN));
  } else {
    std::cout << "Does not claim IEEE conformance" << std::endl;
  }

  if(lim::has_infinity) {
    // Make sure those values are not 0 or similar nonsense.
    // Infinity must compare as if larger than the maximum representable value.
    BOOST_CHECK(infinity > (lim::max)());
    BOOST_CHECK(-infinity < -(lim::max)());
  } else {
    std::cout << "Does not have infinity" << std::endl;
  }

  if(lim::has_quiet_NaN) {
    // NaNs shall always compare "false" when compared for equality
    // If one of these fail, your compiler may be optimizing incorrectly,
    // or the standard library is incorrectly configured.
    BOOST_CHECK(! (qnan == 42));
    BOOST_CHECK(qnan != 42);
    if(lim::is_iec559)
    {
      BOOST_CHECK(! (qnan == qnan));
      BOOST_CHECK(qnan != qnan);
    }

    // The following tests may cause arithmetic traps.
    // BOOST_CHECK(! (qnan < 42));
    // BOOST_CHECK(! (qnan > 42));
    // BOOST_CHECK(! (qnan <= 42));
    // BOOST_CHECK(! (qnan >= 42));
  } else {
    std::cout << "Does not have QNaN" << std::endl;
  }
}


int test_main(int, char*[])
{
  test_integral_limits(bool(), "bool");
  test_integral_limits(char(), "char");
  typedef signed char signed_char;
  test_integral_limits(signed_char(), "signed char");
  typedef unsigned char unsigned_char;
  test_integral_limits(unsigned_char(), "unsigned char");
  test_integral_limits(wchar_t(), "wchar_t");
  test_integral_limits(short(), "short");
  typedef unsigned short unsigned_short;
  test_integral_limits(unsigned_short(), "unsigned short");
  test_integral_limits(int(), "int");
  typedef unsigned int unsigned_int;
  test_integral_limits(unsigned_int(), "unsigned int");
  test_integral_limits(long(), "long");
  typedef unsigned long unsigned_long;
  test_integral_limits(unsigned_long(), "unsigned long");
#if defined(BOOST_HAS_LONG_LONG)
  test_integral_limits(::boost::long_long_type(), "long long");
  test_integral_limits(::boost::ulong_long_type(), "unsigned long long");
#endif
#ifdef BOOST_HAS_MS_INT64
  typedef __int64 long_long2;
  test_integral_limits(long_long2(), "__int64");
  typedef unsigned __int64 unsigned_long_long2;
  test_integral_limits(unsigned_long_long2(), "unsigned __int64");
#endif

  test_float_limits(float(), "float");
  test_float_limits(double(), "double");
  typedef long double long_double;
  test_float_limits(long_double(), "long double");
  // Some compilers don't pay attention to std:3.6.1/5 and issue a
  // warning here if "return 0;" is omitted.
  return 0;
}


