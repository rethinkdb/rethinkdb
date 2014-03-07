//  Copyright John Maddock 2005-2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_PRECISION_INCLUDED
#define BOOST_MATH_TOOLS_PRECISION_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/limits.hpp>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/math/policies/policy.hpp>

// These two are for LDBL_MAN_DIG:
#include <limits.h>
#include <math.h>

namespace boost{ namespace math
{
namespace tools
{
// If T is not specialized, the functions digits, max_value and min_value,
// all get synthesised automatically from std::numeric_limits.
// However, if numeric_limits is not specialised for type RealType,
// for example with NTL::RR type, then you will get a compiler error
// when code tries to use these functions, unless you explicitly specialise them.

// For example if the precision of RealType varies at runtime,
// then numeric_limits support may not be appropriate,
// see boost/math/tools/ntl.hpp  for examples like
// template <> NTL::RR max_value<NTL::RR> ...
// See  Conceptual Requirements for Real Number Types.

template <class T>
inline int digits(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC(T))
{
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
   BOOST_STATIC_ASSERT( ::std::numeric_limits<T>::is_specialized);
   BOOST_STATIC_ASSERT( ::std::numeric_limits<T>::radix == 2 || ::std::numeric_limits<T>::radix == 10);
#else
   BOOST_ASSERT(::std::numeric_limits<T>::is_specialized);
   BOOST_ASSERT(::std::numeric_limits<T>::radix == 2 || ::std::numeric_limits<T>::radix == 10);
#endif
   return std::numeric_limits<T>::radix == 2 
      ? std::numeric_limits<T>::digits
      : ((std::numeric_limits<T>::digits + 1) * 1000L) / 301L;
}

template <class T>
inline T max_value(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE(T))
{
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
   BOOST_STATIC_ASSERT( ::std::numeric_limits<T>::is_specialized);
#else
   BOOST_ASSERT(::std::numeric_limits<T>::is_specialized);
#endif
   return (std::numeric_limits<T>::max)();
} // Also used as a finite 'infinite' value for - and +infinity, for example:
// -max_value<double> = -1.79769e+308, max_value<double> = 1.79769e+308.

template <class T>
inline T min_value(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE(T))
{
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
   BOOST_STATIC_ASSERT( ::std::numeric_limits<T>::is_specialized);
#else
   BOOST_ASSERT(::std::numeric_limits<T>::is_specialized);
#endif
   return (std::numeric_limits<T>::min)();
}

namespace detail{
//
// Logarithmic limits come next, note that although
// we can compute these from the log of the max value
// that is not in general thread safe (if we cache the value)
// so it's better to specialise these:
//
// For type float first:
//
template <class T>
inline T log_max_value(const mpl::int_<128>& BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(T))
{
   return 88.0f;
}

template <class T>
inline T log_min_value(const mpl::int_<128>& BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(T))
{
   return -87.0f;
}
//
// Now double:
//
template <class T>
inline T log_max_value(const mpl::int_<1024>& BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(T))
{
   return 709.0;
}

template <class T>
inline T log_min_value(const mpl::int_<1024>& BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(T))
{
   return -708.0;
}
//
// 80 and 128-bit long doubles:
//
template <class T>
inline T log_max_value(const mpl::int_<16384>& BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(T))
{
   return 11356.0L;
}

template <class T>
inline T log_min_value(const mpl::int_<16384>& BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(T))
{
   return -11355.0L;
}

template <class T>
inline T log_max_value(const mpl::int_<0>& BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(T))
{
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
   BOOST_STATIC_ASSERT( ::std::numeric_limits<T>::is_specialized);
#else
   BOOST_ASSERT(::std::numeric_limits<T>::is_specialized);
#endif
   BOOST_MATH_STD_USING
   static const T val = log((std::numeric_limits<T>::max)());
   return val;
}

template <class T>
inline T log_min_value(const mpl::int_<0>& BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(T))
{
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
   BOOST_STATIC_ASSERT( ::std::numeric_limits<T>::is_specialized);
#else
   BOOST_ASSERT(::std::numeric_limits<T>::is_specialized);
#endif
   BOOST_MATH_STD_USING
   static const T val = log((std::numeric_limits<T>::min)());
   return val;
}

template <class T>
inline T epsilon(const mpl::true_& BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(T))
{
   return std::numeric_limits<T>::epsilon();
}

#if (defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)) && ((LDBL_MANT_DIG == 106) || (__LDBL_MANT_DIG__ == 106))
template <>
inline long double epsilon<long double>(const mpl::true_& BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(long double))
{
   // numeric_limits on Darwin tells lies here.
   // This static assert fails for some unknown reason, so
   // disabled for now...
   // BOOST_STATIC_ASSERT(std::numeric_limits<long double>::digits == 106);
   return 2.4651903288156618919116517665087e-32L;
}
#endif

template <class T>
inline T epsilon(const mpl::false_& BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(T))
{
   BOOST_MATH_STD_USING  // for ADL of std names
   static const T eps = ldexp(static_cast<T>(1), 1-policies::digits<T, policies::policy<> >());
   return eps;
}

} // namespace detail

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4309)
#endif

template <class T>
inline T log_max_value(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE(T))
{
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
   typedef typename mpl::if_c<
      (std::numeric_limits<T>::radix == 2) &&
      (std::numeric_limits<T>::max_exponent == 128
      || std::numeric_limits<T>::max_exponent == 1024
      || std::numeric_limits<T>::max_exponent == 16384),
      mpl::int_<(std::numeric_limits<T>::max_exponent > INT_MAX ? INT_MAX : static_cast<int>(std::numeric_limits<T>::max_exponent))>,
      mpl::int_<0>
   >::type tag_type;
   BOOST_STATIC_ASSERT( ::std::numeric_limits<T>::is_specialized);
   return detail::log_max_value<T>(tag_type());
#else
   BOOST_ASSERT(::std::numeric_limits<T>::is_specialized);
   BOOST_MATH_STD_USING
   static const T val = log((std::numeric_limits<T>::max)());
   return val;
#endif
}

template <class T>
inline T log_min_value(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE(T))
{
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
   typedef typename mpl::if_c<
      (std::numeric_limits<T>::radix == 2) &&
      (std::numeric_limits<T>::max_exponent == 128
      || std::numeric_limits<T>::max_exponent == 1024
      || std::numeric_limits<T>::max_exponent == 16384),
      mpl::int_<(std::numeric_limits<T>::max_exponent > INT_MAX ? INT_MAX : static_cast<int>(std::numeric_limits<T>::max_exponent))>,
      mpl::int_<0>
   >::type tag_type;

   BOOST_STATIC_ASSERT( ::std::numeric_limits<T>::is_specialized);
   return detail::log_min_value<T>(tag_type());
#else
   BOOST_ASSERT(::std::numeric_limits<T>::is_specialized);
   BOOST_MATH_STD_USING
   static const T val = log((std::numeric_limits<T>::min)());
   return val;
#endif
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

template <class T>
inline T epsilon(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC(T))
{
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
   return detail::epsilon<T>(mpl::bool_< ::std::numeric_limits<T>::is_specialized>());
#else
   return ::std::numeric_limits<T>::is_specialized ?
      detail::epsilon<T>(mpl::true_()) :
      detail::epsilon<T>(mpl::false_());
#endif
}

namespace detail{

template <class T>
inline T root_epsilon_imp(const mpl::int_<24>&)
{
   return static_cast<T>(0.00034526698300124390839884978618400831996329879769945L);
}

template <class T>
inline T root_epsilon_imp(const T*, const mpl::int_<53>&)
{
   return static_cast<T>(0.1490116119384765625e-7L);
}

template <class T>
inline T root_epsilon_imp(const T*, const mpl::int_<64>&)
{
   return static_cast<T>(0.32927225399135962333569506281281311031656150598474e-9L);
}

template <class T>
inline T root_epsilon_imp(const T*, const mpl::int_<113>&)
{
   return static_cast<T>(0.1387778780781445675529539585113525390625e-16L);
}

template <class T, class Tag>
inline T root_epsilon_imp(const T*, const Tag&)
{
   BOOST_MATH_STD_USING
   static const T r_eps = sqrt(tools::epsilon<T>());
   return r_eps;
}

template <class T>
inline T forth_root_epsilon_imp(const T*, const mpl::int_<24>&)
{
   return static_cast<T>(0.018581361171917516667460937040007436176452688944747L);
}

template <class T>
inline T forth_root_epsilon_imp(const T*, const mpl::int_<53>&)
{
   return static_cast<T>(0.0001220703125L);
}

template <class T>
inline T forth_root_epsilon_imp(const T*, const mpl::int_<64>&)
{
   return static_cast<T>(0.18145860519450699870567321328132261891067079047605e-4L);
}

template <class T>
inline T forth_root_epsilon_imp(const T*, const mpl::int_<113>&)
{
   return static_cast<T>(0.37252902984619140625e-8L);
}

template <class T, class Tag>
inline T forth_root_epsilon_imp(const T*, const Tag&)
{
   BOOST_MATH_STD_USING
   static const T r_eps = sqrt(sqrt(tools::epsilon<T>()));
   return r_eps;
}

}

template <class T>
inline T root_epsilon()
{
   typedef mpl::int_< (::std::numeric_limits<T>::radix == 2) ? std::numeric_limits<T>::digits : 0> tag_type;
   return detail::root_epsilon_imp(static_cast<T const*>(0), tag_type());
}

template <class T>
inline T forth_root_epsilon()
{
   typedef mpl::int_< (::std::numeric_limits<T>::radix == 2) ? std::numeric_limits<T>::digits : 0> tag_type;
   return detail::forth_root_epsilon_imp(static_cast<T const*>(0), tag_type());
}

} // namespace tools
} // namespace math
} // namespace boost

#endif // BOOST_MATH_TOOLS_PRECISION_INCLUDED

