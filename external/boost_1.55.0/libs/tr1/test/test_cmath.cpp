//  (C) Copyright John Maddock 2008.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// These are the headers included by the Boost.TR1 implementation,
// by including these directly we probe for problems with cyclic
// dependencies when the TR1 headers are in the include search path.

#ifdef TEST_STD_HEADERS
#include <cmath>
#else
#include <boost/tr1/cmath.hpp>
#endif

#include "verify_return.hpp"

int main(int argc, const char* [])
{
   if(argc > 500)
   {
      float f = 0;
      double d = 0;
      long double ld = 0;
      unsigned ui = 0;
      int i = 0;
      long int li = 0;
      bool b = false;
      const char* nan_str = "";
#ifdef BOOST_HAS_LONG_LONG
      long long lli = 0;
#endif
      verify_return_type((std::tr1::acosh)(d), d);
      verify_return_type((std::tr1::acosh)(f), f);
      verify_return_type((std::tr1::acosh)(ld), ld);
      verify_return_type((std::tr1::acoshf)(f), f);
      verify_return_type((std::tr1::acoshl)(ld), ld);
      verify_return_type((std::tr1::asinh)(d), d);
      verify_return_type((std::tr1::asinh)(f), f);
      verify_return_type((std::tr1::asinh)(ld), ld);
      verify_return_type((std::tr1::asinhf)(f), f);
      verify_return_type((std::tr1::asinhl)(ld), ld);
      verify_return_type((std::tr1::atanh)(d), d);
      verify_return_type((std::tr1::atanh)(f), f);
      verify_return_type((std::tr1::atanh)(ld), ld);
      verify_return_type((std::tr1::atanhf)(f), f);
      verify_return_type((std::tr1::atanhl)(ld), ld);
      verify_return_type((std::tr1::cbrt)(d), d);
      verify_return_type((std::tr1::cbrt)(f), f);
      verify_return_type((std::tr1::cbrt)(ld), ld);
      verify_return_type((std::tr1::cbrtf)(f), f);
      verify_return_type((std::tr1::cbrtl)(ld), ld);
      verify_return_type((std::tr1::copysign)(d, d), d);
      verify_return_type((std::tr1::copysign)(f, f), f);
      verify_return_type((std::tr1::copysign)(ld, ld), ld);
      verify_return_type((std::tr1::copysignf)(f, f), f);
      verify_return_type((std::tr1::copysignl)(ld, ld), ld);
      verify_return_type((std::tr1::erf)(d), d);
      verify_return_type((std::tr1::erf)(f), f);
      verify_return_type((std::tr1::erf)(ld), ld);
      verify_return_type((std::tr1::erff)(f), f);
      verify_return_type((std::tr1::erfl)(ld), ld);
      verify_return_type((std::tr1::erfc)(d), d);
      verify_return_type((std::tr1::erfc)(f), f);
      verify_return_type((std::tr1::erfc)(ld), ld);
      verify_return_type((std::tr1::erfcf)(f), f);
      verify_return_type((std::tr1::erfcl)(ld), ld);
#if 0
      verify_return_type((std::tr1::exp2)(d), d);
      verify_return_type((std::tr1::exp2)(f), f);
      verify_return_type((std::tr1::exp2)(ld), ld);
      verify_return_type((std::tr1::exp2f)(f), f);
      verify_return_type((std::tr1::exp2l)(ld), ld);
#endif
      verify_return_type((std::tr1::expm1)(d), d);
      verify_return_type((std::tr1::expm1)(f), f);
      verify_return_type((std::tr1::expm1)(ld), ld);
      verify_return_type((std::tr1::expm1f)(f), f);
      verify_return_type((std::tr1::expm1l)(ld), ld);
#if 0
      verify_return_type((std::tr1::fdim)(d, d), d);
      verify_return_type((std::tr1::fdim)(f, f), f);
      verify_return_type((std::tr1::fdim)(ld, ld), ld);
      verify_return_type((std::tr1::fdimf)(f, f), f);
      verify_return_type((std::tr1::fdiml)(ld, ld), ld);
      verify_return_type((std::tr1::fma)(d, d, d), d);
      verify_return_type((std::tr1::fma)(f, f, f), f);
      verify_return_type((std::tr1::fma)(ld, ld, ld), ld);
      verify_return_type((std::tr1::fmaf)(f, f, f), f);
      verify_return_type((std::tr1::fmal)(ld, ld, ld), ld);
#endif
      verify_return_type((std::tr1::fmax)(d, d), d);
      verify_return_type((std::tr1::fmax)(f, f), f);
      verify_return_type((std::tr1::fmax)(ld, ld), ld);
      verify_return_type((std::tr1::fmaxf)(f, f), f);
      verify_return_type((std::tr1::fmaxl)(ld, ld), ld);
      verify_return_type((std::tr1::fmin)(d, d), d);
      verify_return_type((std::tr1::fmin)(f, f), f);
      verify_return_type((std::tr1::fmin)(ld, ld), ld);
      verify_return_type((std::tr1::fminf)(f, f), f);
      verify_return_type((std::tr1::fminl)(ld, ld), ld);
      verify_return_type((std::tr1::hypot)(d, d), d);
      verify_return_type((std::tr1::hypot)(f, f), f);
      verify_return_type((std::tr1::hypot)(ld, ld), ld);
      verify_return_type((std::tr1::hypotf)(f, f), f);
      verify_return_type((std::tr1::hypotl)(ld, ld), ld);
#if 0
      verify_return_type((std::tr1::ilogb)(d), d);
      verify_return_type((std::tr1::ilogb)(f), f);
      verify_return_type((std::tr1::ilogb)(ld), ld);
      verify_return_type((std::tr1::ilogbf)(f), f);
      verify_return_type((std::tr1::ilogbl)(ld), ld);
#endif
      verify_return_type((std::tr1::lgamma)(d), d);
      verify_return_type((std::tr1::lgamma)(f), f);
      verify_return_type((std::tr1::lgamma)(ld), ld);
      verify_return_type((std::tr1::lgammaf)(f), f);
      verify_return_type((std::tr1::lgammal)(ld), ld);
#if 0
      verify_return_type((std::tr1::llrint)(d), d);
      verify_return_type((std::tr1::llrint)(f), f);
      verify_return_type((std::tr1::llrint)(ld), ld);
      verify_return_type((std::tr1::llrintf)(f), f);
      verify_return_type((std::tr1::llrintl)(ld), ld);
#endif
#ifdef BOOST_HAS_LONG_LONG
      verify_return_type((std::tr1::llround)(d), lli);
      verify_return_type((std::tr1::llround)(f), lli);
      verify_return_type((std::tr1::llround)(ld), lli);
      verify_return_type((std::tr1::llroundf)(f), lli);
      verify_return_type((std::tr1::llroundl)(ld), lli);
#endif
      verify_return_type((std::tr1::log1p)(d), d);
      verify_return_type((std::tr1::log1p)(f), f);
      verify_return_type((std::tr1::log1p)(ld), ld);
      verify_return_type((std::tr1::log1pf)(f), f);
      verify_return_type((std::tr1::log1pl)(ld), ld);
#if 0
      verify_return_type((std::tr1::log2)(d), d);
      verify_return_type((std::tr1::log2)(f), f);
      verify_return_type((std::tr1::log2)(ld), ld);
      verify_return_type((std::tr1::log2f)(f), f);
      verify_return_type((std::tr1::log2l)(ld), ld);
      verify_return_type((std::tr1::logb)(d), d);
      verify_return_type((std::tr1::logb)(f), f);
      verify_return_type((std::tr1::logb)(ld), ld);
      verify_return_type((std::tr1::logbf)(f), f);
      verify_return_type((std::tr1::logbl)(ld), ld);
      verify_return_type((std::tr1::lrint)(d), d);
      verify_return_type((std::tr1::lrint)(f), f);
      verify_return_type((std::tr1::lrint)(ld), ld);
      verify_return_type((std::tr1::lrintf)(f), f);
      verify_return_type((std::tr1::lrintl)(ld), ld);
#endif
      verify_return_type((std::tr1::lround)(d), li);
      verify_return_type((std::tr1::lround)(f), li);
      verify_return_type((std::tr1::lround)(ld), li);
      verify_return_type((std::tr1::lroundf)(f), li);
      verify_return_type((std::tr1::lroundl)(ld), li);
#if 0
      verify_return_type((std::tr1::nan)(nan_str), d);
      verify_return_type((std::tr1::nanf)(nan_str), f);
      verify_return_type((std::tr1::nanl)(nan_str), ld);
      verify_return_type((std::tr1::nearbyint)(d), d);
      verify_return_type((std::tr1::nearbyint)(f), f);
      verify_return_type((std::tr1::nearbyint)(ld), ld);
      verify_return_type((std::tr1::nearbyintf)(f), f);
      verify_return_type((std::tr1::nearbyintl)(ld), ld);
#endif
      verify_return_type((std::tr1::nextafter)(d, d), d);
      verify_return_type((std::tr1::nextafter)(f, f), f);
      verify_return_type((std::tr1::nextafter)(ld, ld), ld);
      verify_return_type((std::tr1::nextafterf)(f, f), f);
      verify_return_type((std::tr1::nextafterl)(ld, ld), ld);
      verify_return_type((std::tr1::nexttoward)(d, d), d);
      verify_return_type((std::tr1::nexttoward)(f, f), f);
      verify_return_type((std::tr1::nexttoward)(ld, ld), ld);
      verify_return_type((std::tr1::nexttoward)(f, ld), ld);
      verify_return_type((std::tr1::nexttoward)(f, d), d);
      verify_return_type((std::tr1::nexttowardf)(f, ld), f);
      verify_return_type((std::tr1::nexttowardl)(ld, ld), ld);
#if 0
      verify_return_type((std::tr1::remainder)(d, d), d);
      verify_return_type((std::tr1::remainder)(f, f), f);
      verify_return_type((std::tr1::remainder)(ld, ld), ld);
      verify_return_type((std::tr1::remainderf)(f, f), f);
      verify_return_type((std::tr1::remainderl)(ld, ld), ld);
      verify_return_type((std::tr1::remquo)(d, d, &i), d);
      verify_return_type((std::tr1::remquo)(f, f, &i), f);
      verify_return_type((std::tr1::remquo)(ld, ld, &i), ld);
      verify_return_type((std::tr1::remquof)(f, f, &i), f);
      verify_return_type((std::tr1::remquol)(ld, ld, &i), ld);
      verify_return_type((std::tr1::rint)(d), d);
      verify_return_type((std::tr1::rint)(f), f);
      verify_return_type((std::tr1::rint)(ld), ld);
      verify_return_type((std::tr1::rintf)(f), f);
      verify_return_type((std::tr1::rintl)(ld), ld);
#endif
      verify_return_type((std::tr1::round)(d), d);
      verify_return_type((std::tr1::round)(f), f);
      verify_return_type((std::tr1::round)(ld), ld);
      verify_return_type((std::tr1::roundf)(f), f);
      verify_return_type((std::tr1::roundl)(ld), ld);
#if 0
      verify_return_type((std::tr1::scalbln)(d, li), d);
      verify_return_type((std::tr1::scalbln)(f, li), f);
      verify_return_type((std::tr1::scalbln)(ld, li), ld);
      verify_return_type((std::tr1::scalblnf)(f, li), f);
      verify_return_type((std::tr1::scalblnl)(ld, li), ld);
      verify_return_type((std::tr1::scalbn)(d, i), d);
      verify_return_type((std::tr1::scalbn)(f, i), f);
      verify_return_type((std::tr1::scalbn)(ld, i), ld);
      verify_return_type((std::tr1::scalbnf)(f, i), f);
      verify_return_type((std::tr1::scalbnl)(ld, i), ld);
#endif
      verify_return_type((std::tr1::tgamma)(d), d);
      verify_return_type((std::tr1::tgamma)(f), f);
      verify_return_type((std::tr1::tgamma)(ld), ld);
      verify_return_type((std::tr1::tgammaf)(f), f);
      verify_return_type((std::tr1::tgammal)(ld), ld);
      verify_return_type((std::tr1::trunc)(d), d);
      verify_return_type((std::tr1::trunc)(f), f);
      verify_return_type((std::tr1::trunc)(ld), ld);
      verify_return_type((std::tr1::truncf)(f), f);
      verify_return_type((std::tr1::truncl)(ld), ld);
      verify_return_type((std::tr1::signbit)(d), b);
      verify_return_type((std::tr1::signbit)(f), b);
      verify_return_type((std::tr1::signbit)(ld), b);
      verify_return_type((std::tr1::fpclassify)(d), i);
      verify_return_type((std::tr1::fpclassify)(f), i);
      verify_return_type((std::tr1::fpclassify)(ld), i);
      verify_return_type((std::tr1::isfinite)(d), b);
      verify_return_type((std::tr1::isfinite)(f), b);
      verify_return_type((std::tr1::isfinite)(ld), b);
      verify_return_type((std::tr1::isinf)(d), b);
      verify_return_type((std::tr1::isinf)(f), b);
      verify_return_type((std::tr1::isinf)(ld), b);
      verify_return_type((std::tr1::isnan)(d), b);
      verify_return_type((std::tr1::isnan)(f), b);
      verify_return_type((std::tr1::isnan)(ld), b);
      verify_return_type((std::tr1::isnormal)(d), b);
      verify_return_type((std::tr1::isnormal)(f), b);
      verify_return_type((std::tr1::isnormal)(ld), b);
#if 0
      verify_return_type((std::tr1::isgreater)(d, d), b);
      verify_return_type((std::tr1::isgreater)(f, f), b);
      verify_return_type((std::tr1::isgreater)(ld, ld), b);
      verify_return_type((std::tr1::isgreaterequal)(d, d), b);
      verify_return_type((std::tr1::isgreaterequal)(f, f), b);
      verify_return_type((std::tr1::isgreaterequal)(ld, ld), b);
      verify_return_type((std::tr1::isless)(d, d), b);
      verify_return_type((std::tr1::isless)(f, f), b);
      verify_return_type((std::tr1::isless)(ld, ld), b);
      verify_return_type((std::tr1::islessequal)(d, d), b);
      verify_return_type((std::tr1::islessequal)(f, f), b);
      verify_return_type((std::tr1::islessequal)(ld, ld), b);
      verify_return_type((std::tr1::islessgreater)(d, d), b);
      verify_return_type((std::tr1::islessgreater)(f, f), b);
      verify_return_type((std::tr1::islessgreater)(ld, ld), b);
      verify_return_type((std::tr1::isunordered)(d, d), b);
      verify_return_type((std::tr1::isunordered)(f, f), b);
      verify_return_type((std::tr1::isunordered)(ld, ld), b);
#endif

      // [5.2.1.1] associated Laguerre polynomials:
      verify_return_type((std::tr1::assoc_laguerre)(ui, ui, d), d);
      verify_return_type((std::tr1::assoc_laguerre)(ui, ui, f), f);
      verify_return_type((std::tr1::assoc_laguerre)(ui, ui, ld), ld);
      verify_return_type((std::tr1::assoc_laguerref)(ui, ui, f), f);
      verify_return_type((std::tr1::assoc_laguerrel)(ui, ui, ld), ld);
      // [5.2.1.2] associated Legendre functions:
      verify_return_type((std::tr1::assoc_legendre)(ui, ui, d), d);
      verify_return_type((std::tr1::assoc_legendre)(ui, ui, f), f);
      verify_return_type((std::tr1::assoc_legendre)(ui, ui, ld), ld);
      verify_return_type((std::tr1::assoc_legendref)(ui, ui, f), f);
      verify_return_type((std::tr1::assoc_legendrel)(ui, ui, ld), ld);
      // [5.2.1.3] beta function:
      verify_return_type((std::tr1::beta)(d, d), d);
      verify_return_type((std::tr1::beta)(f, f), f);
      verify_return_type((std::tr1::beta)(ld, ld), ld);
      verify_return_type((std::tr1::betaf)(f, f), f);
      verify_return_type((std::tr1::betal)(ld, ld), ld);
      // [5.2.1.4] (complete) elliptic integral of the first kind:
      verify_return_type((std::tr1::comp_ellint_1)(d), d);
      verify_return_type((std::tr1::comp_ellint_1)(f), f);
      verify_return_type((std::tr1::comp_ellint_1)(ld), ld);
      verify_return_type((std::tr1::comp_ellint_1f)(f), f);
      verify_return_type((std::tr1::comp_ellint_1l)(ld), ld);
      // [5.2.1.5] (complete) elliptic integral of the second kind:
      verify_return_type((std::tr1::comp_ellint_2)(d), d);
      verify_return_type((std::tr1::comp_ellint_2)(f), f);
      verify_return_type((std::tr1::comp_ellint_2)(ld), ld);
      verify_return_type((std::tr1::comp_ellint_2f)(f), f);
      verify_return_type((std::tr1::comp_ellint_2l)(ld), ld);
      // [5.2.1.6] (complete) elliptic integral of the third kind:
      verify_return_type((std::tr1::comp_ellint_3)(d, d), d);
      verify_return_type((std::tr1::comp_ellint_3)(f, f), f);
      verify_return_type((std::tr1::comp_ellint_3)(ld, ld), ld);
      verify_return_type((std::tr1::comp_ellint_3f)(f, f), f);
      verify_return_type((std::tr1::comp_ellint_3l)(ld, ld), ld);
#if 0
      // [5.2.1.7] confluent hypergeometric functions:
      verify_return_type((std::tr1::conf_hyperg)(d, d, d), d);
      verify_return_type((std::tr1::conf_hyperg)(f, f, f), f);
      verify_return_type((std::tr1::conf_hyperg)(ld, ld, ld), ld);
      verify_return_type((std::tr1::conf_hypergf)(f, f, f), f);
      verify_return_type((std::tr1::conf_hypergl)(ld, ld, ld), ld);
#endif
      // [5.2.1.8] regular modified cylindrical Bessel functions:
      verify_return_type((std::tr1::cyl_bessel_i)(d, d), d);
      verify_return_type((std::tr1::cyl_bessel_i)(f, f), f);
      verify_return_type((std::tr1::cyl_bessel_i)(ld, ld), ld);
      verify_return_type((std::tr1::cyl_bessel_if)(f, f), f);
      verify_return_type((std::tr1::cyl_bessel_il)(ld, ld), ld);
      // [5.2.1.9] cylindrical Bessel functions (of the first kind):
      verify_return_type((std::tr1::cyl_bessel_j)(d, d), d);
      verify_return_type((std::tr1::cyl_bessel_j)(f, f), f);
      verify_return_type((std::tr1::cyl_bessel_j)(ld, ld), ld);
      verify_return_type((std::tr1::cyl_bessel_jf)(f, f), f);
      verify_return_type((std::tr1::cyl_bessel_jl)(ld, ld), ld);
      // [5.2.1.10] irregular modified cylindrical Bessel functions:
      verify_return_type((std::tr1::cyl_bessel_k)(d, d), d);
      verify_return_type((std::tr1::cyl_bessel_k)(f, f), f);
      verify_return_type((std::tr1::cyl_bessel_k)(ld, ld), ld);
      verify_return_type((std::tr1::cyl_bessel_kf)(f, f), f);
      verify_return_type((std::tr1::cyl_bessel_kl)(ld, ld), ld);
      // [5.2.1.11] cylindrical Neumann functions;
      // cylindrical Bessel functions (of the second kind):
      verify_return_type((std::tr1::cyl_neumann)(d, d), d);
      verify_return_type((std::tr1::cyl_neumann)(f, f), f);
      verify_return_type((std::tr1::cyl_neumann)(ld, ld), ld);
      verify_return_type((std::tr1::cyl_neumannf)(f, f), f);
      verify_return_type((std::tr1::cyl_neumannl)(ld, ld), ld);
      // [5.2.1.12] (incomplete) elliptic integral of the first kind:
      verify_return_type((std::tr1::ellint_1)(d, d), d);
      verify_return_type((std::tr1::ellint_1)(f, f), f);
      verify_return_type((std::tr1::ellint_1)(ld, ld), ld);
      verify_return_type((std::tr1::ellint_1f)(f, f), f);
      verify_return_type((std::tr1::ellint_1l)(ld, ld), ld);
      // [5.2.1.13] (incomplete) elliptic integral of the second kind:
      verify_return_type((std::tr1::ellint_2)(d, d), d);
      verify_return_type((std::tr1::ellint_2)(f, f), f);
      verify_return_type((std::tr1::ellint_2)(ld, ld), ld);
      verify_return_type((std::tr1::ellint_2f)(f, f), f);
      verify_return_type((std::tr1::ellint_2l)(ld, ld), ld);
      // [5.2.1.14] (incomplete) elliptic integral of the third kind:
      verify_return_type((std::tr1::ellint_3)(d, d, d), d);
      verify_return_type((std::tr1::ellint_3)(f, f, f), f);
      verify_return_type((std::tr1::ellint_3)(ld, ld, ld), ld);
      verify_return_type((std::tr1::ellint_3f)(f, f, f), f);
      verify_return_type((std::tr1::ellint_3l)(ld, ld, ld), ld);
      // [5.2.1.15] exponential integral:
      verify_return_type((std::tr1::expint)(d), d);
      verify_return_type((std::tr1::expint)(f), f);
      verify_return_type((std::tr1::expint)(ld), ld);
      verify_return_type((std::tr1::expintf)(f), f);
      verify_return_type((std::tr1::expintl)(ld), ld);
      // [5.2.1.16] Hermite polynomials:
      verify_return_type((std::tr1::hermite)(ui, d), d);
      verify_return_type((std::tr1::hermite)(ui, f), f);
      verify_return_type((std::tr1::hermite)(ui, ld), ld);
      verify_return_type((std::tr1::hermitef)(ui, f), f);
      verify_return_type((std::tr1::hermitel)(ui, ld), ld);
      // [5.2.1.17] hypergeometric functions:
#if 0
      verify_return_type((std::tr1::hyperg)(d, d, d, d), d);
      verify_return_type((std::tr1::hyperg)(f, f, f, f), f);
      verify_return_type((std::tr1::hyperg)(ld, ld, ld, ld), ld);
      verify_return_type((std::tr1::hypergf)(f, f, f, f), f);
      verify_return_type((std::tr1::hypergl)(ld, ld, ld, ld), ld);
#endif
      // [5.2.1.18] Laguerre polynomials:
      verify_return_type((std::tr1::laguerre)(ui, d), d);
      verify_return_type((std::tr1::laguerre)(ui, f), f);
      verify_return_type((std::tr1::laguerre)(ui, ld), ld);
      verify_return_type((std::tr1::laguerref)(ui, f), f);
      verify_return_type((std::tr1::laguerrel)(ui, ld), ld);
      // [5.2.1.19] Legendre polynomials:
      verify_return_type((std::tr1::legendre)(ui, d), d);
      verify_return_type((std::tr1::legendre)(ui, f), f);
      verify_return_type((std::tr1::legendre)(ui, ld), ld);
      verify_return_type((std::tr1::legendref)(ui, f), f);
      verify_return_type((std::tr1::legendrel)(ui, ld), ld);
      // [5.2.1.20] Riemann zeta function:
      verify_return_type((std::tr1::riemann_zeta)(d), d);
      verify_return_type((std::tr1::riemann_zeta)(f), f);
      verify_return_type((std::tr1::riemann_zeta)(ld), ld);
      verify_return_type((std::tr1::riemann_zetaf)(f), f);
      verify_return_type((std::tr1::riemann_zetal)(ld), ld);
      // [5.2.1.21] spherical Bessel functions (of the first kind):
      verify_return_type((std::tr1::sph_bessel)(ui, d), d);
      verify_return_type((std::tr1::sph_bessel)(ui, f), f);
      verify_return_type((std::tr1::sph_bessel)(ui, ld), ld);
      verify_return_type((std::tr1::sph_besself)(ui, f), f);
      verify_return_type((std::tr1::sph_bessell)(ui, ld), ld);
      // [5.2.1.22] spherical associated Legendre functions:
      verify_return_type((std::tr1::sph_legendre)(ui, ui, d), d);
      verify_return_type((std::tr1::sph_legendre)(ui, ui, f), f);
      verify_return_type((std::tr1::sph_legendre)(ui, ui, ld), ld);
      verify_return_type((std::tr1::sph_legendref)(ui, ui, f), f);
      verify_return_type((std::tr1::sph_legendrel)(ui, ui, ld), ld);
      // [5.2.1.23] spherical Neumann functions;
      // spherical Bessel functions (of the second kind):
      verify_return_type((std::tr1::sph_neumann)(ui, d), d);
      verify_return_type((std::tr1::sph_neumann)(ui, f), f);
      verify_return_type((std::tr1::sph_neumann)(ui, ld), ld);
      verify_return_type((std::tr1::sph_neumannf)(ui, f), f);
      verify_return_type((std::tr1::sph_neumannl)(ui, ld), ld);
   }
   return 0;
}




