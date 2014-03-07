// Copyright Paul Bristow 2007, 2011.
// Copyright John Maddock 2006, 2011.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// test_constants.cpp

// Check values of constants are drawn from an independent source, or calculated.
// Both must be at long double precision for the most precise compilers floating-point implementation.
// So all values use static_cast<RealType>() of values at least 40 decimal digits
// and that have suffix L to ensure floating-point type is long double.

// Steve Moshier's command interpreter V1.3 100 digits calculator used for some values.

#ifdef _MSC_VER
#  pragma warning(disable : 4127) // conditional expression is constant.
#endif

#include <boost/math/concepts/real_concept.hpp> // for real_concept
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp> // Boost.Test
#include <boost/test/floating_point_comparison.hpp>

#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/test.hpp>
#include <boost/static_assert.hpp>
#include <boost/utility/enable_if.hpp>

// Check at compile time that the construction method for constants of type float, is "construct from a float", or "construct from a double", ...
BOOST_STATIC_ASSERT((boost::is_same<boost::math::constants::construction_traits<float, boost::math::policies::policy<> >::type, boost::mpl::int_<boost::math::constants::construct_from_float> >::value));
BOOST_STATIC_ASSERT((boost::is_same<boost::math::constants::construction_traits<double, boost::math::policies::policy<> >::type, boost::mpl::int_<boost::math::constants::construct_from_double> >::value));
BOOST_STATIC_ASSERT((boost::is_same<boost::math::constants::construction_traits<long double, boost::math::policies::policy<> >::type, boost::mpl::int_<(sizeof(double) == sizeof(long double) ? boost::math::constants::construct_from_double : boost::math::constants::construct_from_long_double)> >::value));
BOOST_STATIC_ASSERT((boost::is_same<boost::math::constants::construction_traits<boost::math::concepts::real_concept, boost::math::policies::policy<> >::type, boost::mpl::int_<0> >::value));

// Policy to set precision at maximum possible using long double.
typedef boost::math::policies::policy<boost::math::policies::digits2<std::numeric_limits<long double>::digits> > real_concept_policy_1;
// Policy with precision +2 (could be any reasonable value),
// forces the precision of the policy to be greater than
// that of a long double, and therefore triggers different code (construct from string).
#ifdef BOOST_MATH_USE_FLOAT128
typedef boost::math::policies::policy<boost::math::policies::digits2<115> > real_concept_policy_2;
#else
typedef boost::math::policies::policy<boost::math::policies::digits2<std::numeric_limits<long double>::digits + 2> > real_concept_policy_2;
#endif
// Policy with precision greater than the string representations, forces computation of values (i.e. different code path):
typedef boost::math::policies::policy<boost::math::policies::digits2<400> > real_concept_policy_3;

BOOST_STATIC_ASSERT((boost::is_same<boost::math::constants::construction_traits<boost::math::concepts::real_concept, real_concept_policy_1 >::type, boost::mpl::int_<(sizeof(double) == sizeof(long double) ? boost::math::constants::construct_from_double : boost::math::constants::construct_from_long_double) > >::value));
BOOST_STATIC_ASSERT((boost::is_same<boost::math::constants::construction_traits<boost::math::concepts::real_concept, real_concept_policy_2 >::type, boost::mpl::int_<boost::math::constants::construct_from_string> >::value));
BOOST_STATIC_ASSERT((boost::math::constants::construction_traits<boost::math::concepts::real_concept, real_concept_policy_3>::type::value >= 5));

#ifndef BOOST_NO_CXX11_CONSTEXPR

constexpr float fval = boost::math::constants::pi<float>();
constexpr double dval = boost::math::constants::pi<double>();
constexpr long double ldval = boost::math::constants::pi<long double>();

#endif

// We need to declare a conceptual type whose precision is unknown at
// compile time, and is so enormous when checked at runtime,
// that we're forced to calculate the values of the constants ourselves.

namespace boost{ namespace math{ namespace concepts{

class big_real_concept : public real_concept
{
public:
   big_real_concept() {}
   template <class T>
   big_real_concept(const T& t, typename enable_if<is_convertible<T, real_concept> >::type* = 0) : real_concept(t) {}
};

inline int itrunc(const big_real_concept& val)
{
   BOOST_MATH_STD_USING
   return itrunc(val.value());
}

}
namespace tools{

template <>
inline int digits<concepts::big_real_concept>(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC(T))
{
   return 2 * boost::math::constants::max_string_digits;
}

}}}

template <class RealType>
void test_spots(RealType)
{
   // Basic sanity checks for constants,
   // where template parameter RealType can be float, double, long double,
   // or real_concept, a prototype for user-defined floating-point types.

   // Parameter RealType is only used to communicate the RealType,
  //  and is an arbitrary zero for all tests.
   //
   // Actual tolerance is never really smaller than epsilon for long double,
   // because it's just a wrapper around a long double,
   // so although it's pretending to be something else (in order to exercise our code),
   // it can never really have precision greater than a long double.

  typedef typename boost::math::constants::construction_traits<RealType, boost::math::policies::policy<> >::type construction_type;
   RealType tolerance = (std::max)(static_cast<RealType>(boost::math::tools::epsilon<long double>()), boost::math::tools::epsilon<RealType>()) * 2;  // double
   if((construction_type::value == 0) && (boost::math::tools::digits<RealType>() > boost::math::constants::max_string_digits))
      tolerance *= 30;  // Allow a little extra tolerance
   // for calculated (perhaps using a series representation) constants.
   std::cout << "Tolerance for type " << typeid(RealType).name()  << " is " << tolerance << "." << std::endl;

   //typedef typename boost::math::policies::precision<RealType, boost::math::policies::policy<> >::type t1;
   // A precision of zero means we don't know what the precision of this type is until runtime.
   //std::cout << "Precision for type " << typeid(RealType).name()  << " is " << t1::value << "." << std::endl;

   using namespace boost::math::constants;
   BOOST_MATH_STD_USING

   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L, pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(3.14159265358979323846264338327950288419716939937510L), root_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(3.14159265358979323846264338327950288419716939937510L/2), root_half_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(3.14159265358979323846264338327950288419716939937510L * 2), root_two_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(log(4.0L)), root_ln_four<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(2.71828182845904523536028747135266249775724709369995L, e<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(0.5L, half<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(0.57721566490153286060651209008240243104259335L, euler<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(2.0L), root_two<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log(2.0L), ln_two<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log(10.0L), ln_ten<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log(log(2.0L)), ln_ln_two<RealType>(), tolerance);

   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(1)/3, third<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(2)/3, twothirds<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(0.14159265358979323846264338327950288419716939937510L, pi_minus_three<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(4.L - 3.14159265358979323846264338327950288419716939937510L, four_minus_pi<RealType>(), tolerance);
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   BOOST_CHECK_CLOSE_FRACTION(pow((4 - 3.14159265358979323846264338327950288419716939937510L), 1.5L), pow23_four_minus_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(pow((3.14159265358979323846264338327950288419716939937510L), 2.71828182845904523536028747135266249775724709369995L), pi_pow_e<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(pow((3.14159265358979323846264338327950288419716939937510L), 0.33333333333333333333333333333333333333333333333333L), cbrt_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(exp(-0.5L), exp_minus_half<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(pow(2.71828182845904523536028747135266249775724709369995L, 3.14159265358979323846264338327950288419716939937510L), e_pow_pi<RealType>(), tolerance);


#else // Only double, so no suffix L.
   BOOST_CHECK_CLOSE_FRACTION(pow((4 - 3.14159265358979323846264338327950288419716939937510), 1.5), pow23_four_minus_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(pow((3.14159265358979323846264338327950288419716939937510), 2.71828182845904523536028747135266249775724709369995), pi_pow_e<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(pow((3.14159265358979323846264338327950288419716939937510), 0.33333333333333333333333333333333333333333333333333), cbrt_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(exp(-0.5), exp_minus_half<RealType>(), tolerance);
#endif
   // Rational fractions.
   BOOST_CHECK_CLOSE_FRACTION(0.333333333333333333333333333333333333333L, third<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(0.666666666666666666666666666666666666667L, two_thirds<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(0.75L, three_quarters<RealType>(), tolerance);
   // Two and related.
   BOOST_CHECK_CLOSE_FRACTION(sqrt(2.L), root_two<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(3.L), root_three<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(2.L)/2, half_root_two<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log(2.L), ln_two<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log(log(2.0L)), ln_ln_two<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(log(4.0L)), root_ln_four<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1/sqrt(2.0L), one_div_root_two<RealType>(), tolerance);

   // pi.
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L, pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L/2, half_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L/3, third_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L/6, sixth_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(2 * 3.14159265358979323846264338327950288419716939937510L, two_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(3 * 3.14159265358979323846264338327950288419716939937510L / 4, three_quarters_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(4 * 3.14159265358979323846264338327950288419716939937510L / 3, four_thirds_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1 / (2 * 3.14159265358979323846264338327950288419716939937510L), one_div_two_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(3.14159265358979323846264338327950288419716939937510L), root_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(3.14159265358979323846264338327950288419716939937510L / 2), root_half_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(2 * 3.14159265358979323846264338327950288419716939937510L), root_two_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1 / sqrt(3.14159265358979323846264338327950288419716939937510L), one_div_root_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1 / sqrt(2 * 3.14159265358979323846264338327950288419716939937510L), one_div_root_two_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(1. / 3.14159265358979323846264338327950288419716939937510L), root_one_div_pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L - 3.L, pi_minus_three<RealType>(), tolerance * 2 ); // tolerance * 2 because of cancellation loss.
   BOOST_CHECK_CLOSE_FRACTION(4.L - 3.14159265358979323846264338327950288419716939937510L, four_minus_pi<RealType>(), tolerance );
   //  BOOST_CHECK_CLOSE_FRACTION(pow((4 - 3.14159265358979323846264338327950288419716939937510L), 1.5L), pow23_four_minus_pi<RealType>(), tolerance); See above.
   //
   BOOST_CHECK_CLOSE_FRACTION(pow((3.14159265358979323846264338327950288419716939937510L), 2.71828182845904523536028747135266249775724709369995L), pi_pow_e<RealType>(), tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L, pi_sqr<RealType>(), tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L/6, pi_sqr_div_six<RealType>(), tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L, pi_cubed<RealType>(), tolerance);  // See above.

   // BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L, cbrt_pi<RealType>(), tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(cbrt_pi<RealType>() * cbrt_pi<RealType>() * cbrt_pi<RealType>(), pi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION((1)/cbrt_pi<RealType>(), one_div_cbrt_pi<RealType>(), tolerance);

   // Euler
   BOOST_CHECK_CLOSE_FRACTION(2.71828182845904523536028747135266249775724709369995L, e<RealType>(), tolerance);

   //BOOST_CHECK_CLOSE_FRACTION(exp(-0.5L), exp_minus_half<RealType>(), tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(pow(e<RealType>(), pi<RealType>()), e_pow_pi<RealType>(), tolerance); // See also above.
   BOOST_CHECK_CLOSE_FRACTION(sqrt(e<RealType>()), root_e<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log10(e<RealType>()), log10_e<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1/log10(e<RealType>()), one_div_log10_e<RealType>(), tolerance);

   // Trigonmetric
   BOOST_CHECK_CLOSE_FRACTION(pi<RealType>()/180, degree<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(180 / pi<RealType>(), radian<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sin(1.L), sin_one<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(cos(1.L), cos_one<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sinh(1.L), sinh_one<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(cosh(1.L), cosh_one<RealType>(), tolerance);

   // Phi
   BOOST_CHECK_CLOSE_FRACTION((1.L + sqrt(5.L)) /2, phi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log((1.L + sqrt(5.L)) /2), ln_phi<RealType>(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1.L / log((1.L + sqrt(5.L)) /2), one_div_ln_phi<RealType>(), tolerance);

   //Euler's Gamma
   BOOST_CHECK_CLOSE_FRACTION(0.57721566490153286060651209008240243104215933593992L, euler<RealType>(), tolerance); // (sequence A001620 in OEIS).
   BOOST_CHECK_CLOSE_FRACTION(1.L/ 0.57721566490153286060651209008240243104215933593992L, one_div_euler<RealType>(), tolerance); // (from sequence A001620 in OEIS).
   BOOST_CHECK_CLOSE_FRACTION(0.57721566490153286060651209008240243104215933593992L * 0.57721566490153286060651209008240243104215933593992L, euler_sqr<RealType>(), tolerance); // (from sequence A001620 in OEIS).

   // Misc
   BOOST_CHECK_CLOSE_FRACTION(1.644934066848226436472415166646025189218949901206L, zeta_two<RealType>(), tolerance); // A013661 as a constant (usually base 10) in OEIS.
   BOOST_CHECK_CLOSE_FRACTION(1.20205690315959428539973816151144999076498629234049888179227L, zeta_three<RealType>(), tolerance); // (sequence A002117 in OEIS)
   BOOST_CHECK_CLOSE_FRACTION(.91596559417721901505460351493238411077414937428167213L, catalan<RealType>(), tolerance); // A006752 as a constant in OEIS.
   BOOST_CHECK_CLOSE_FRACTION(1.1395470994046486574927930193898461120875997958365518247216557100852480077060706857071875468869385150L, extreme_value_skewness<RealType>(), tolerance); //  Mathematica: N[12 Sqrt[6]  Zeta[3]/Pi^3, 1101]
   BOOST_CHECK_CLOSE_FRACTION(0.6311106578189371381918993515442277798440422031347194976580945856929268196174737254599050270325373067L, rayleigh_skewness<RealType>(), tolerance); // Mathematica: N[2 Sqrt[Pi] (Pi - 3)/((4 - Pi)^(3/2)), 1100]
   BOOST_CHECK_CLOSE_FRACTION(2.450893006876380628486604106197544154e-01L, rayleigh_kurtosis_excess<RealType>(), tolerance * 2);
   BOOST_CHECK_CLOSE_FRACTION(2.68545200106530644530971483548179569382038229399446295305115234555721885953715200280114117493184769799515L, khinchin<RealType>(), tolerance ); // A002210 as a constant https://oeis.org/A002210/constant
   BOOST_CHECK_CLOSE_FRACTION(1.2824271291006226368753425688697917277676889273250011L, glaisher<RealType>(), tolerance ); // https://oeis.org/A074962/constant

   //
   // Last of all come the test cases that behave differently if we're calculating the constants on the fly:
   //
   if(boost::math::tools::digits<RealType>() > boost::math::constants::max_string_digits)
   {
      // This suffers from cancellation error, so increased tolerance:
      BOOST_CHECK_CLOSE_FRACTION(static_cast<RealType>(4. - 3.14159265358979323846264338327950288419716939937510L), four_minus_pi<RealType>(), tolerance * 3);
      BOOST_CHECK_CLOSE_FRACTION(static_cast<RealType>(0.14159265358979323846264338327950288419716939937510L), pi_minus_three<RealType>(), tolerance * 3);
   }
   else
   {
      BOOST_CHECK_CLOSE_FRACTION(static_cast<RealType>(4. - 3.14159265358979323846264338327950288419716939937510L), four_minus_pi<RealType>(), tolerance);
      BOOST_CHECK_CLOSE_FRACTION(static_cast<RealType>(0.14159265358979323846264338327950288419716939937510L), pi_minus_three<RealType>(), tolerance);
   }
} // template <class RealType>void test_spots(RealType)

void test_float_spots()
{
   // Basic sanity checks for constants in boost::math::float_constants::
   // for example: boost::math::float_constants::pi
   // (rather than boost::math::constants::pi<float>() ).

   float tolerance = boost::math::tools::epsilon<float>() * 2;

   using namespace boost::math::float_constants;
   BOOST_MATH_STD_USING

   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(3.14159265358979323846264338327950288419716939937510F), pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(3.14159265358979323846264338327950288419716939937510F)), root_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(3.14159265358979323846264338327950288419716939937510F/2)), root_half_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(3.14159265358979323846264338327950288419716939937510F * 2)), root_two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(log(4.0F))), root_ln_four, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(2.71828182845904523536028747135266249775724709369995F), e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(0.5), half, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(0.57721566490153286060651209008240243104259335F), euler, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(2.0F)), root_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(log(2.0F)), ln_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(log(log(2.0F))), ln_ln_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(1)/3, third, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(2)/3, twothirds, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(0.14159265358979323846264338327950288419716939937510F), pi_minus_three, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(4.F - 3.14159265358979323846264338327950288419716939937510F), four_minus_pi, tolerance);
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(pow((4 - 3.14159265358979323846264338327950288419716939937510F), 1.5F)), pow23_four_minus_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(pow((3.14159265358979323846264338327950288419716939937510F), 2.71828182845904523536028747135266249775724709369995F)), pi_pow_e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(pow((3.14159265358979323846264338327950288419716939937510F), 0.33333333333333333333333333333333333333333333333333F)), cbrt_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(exp(-0.5F)), exp_minus_half, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(pow(2.71828182845904523536028747135266249775724709369995F, 3.14159265358979323846264338327950288419716939937510F)), e_pow_pi, tolerance);


#else // Only double, so no suffix F.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(pow((4 - 3.14159265358979323846264338327950288419716939937510), 1.5)), pow23_four_minus_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(pow((3.14159265358979323846264338327950288419716939937510), 2.71828182845904523536028747135266249775724709369995)), pi_pow_e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(pow((3.14159265358979323846264338327950288419716939937510), 0.33333333333333333333333333333333333333333333333333)), cbrt_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(exp(-0.5)), exp_minus_half, tolerance);
#endif
   // Rational fractions.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(0.333333333333333333333333333333333333333F), third, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(0.666666666666666666666666666666666666667F), two_thirds, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(0.75F), three_quarters, tolerance);
   // Two and related.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(2.F)), root_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(3.F)), root_three, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(2.F)/2), half_root_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(log(2.F)), ln_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(log(log(2.0F))), ln_ln_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(log(4.0F))), root_ln_four, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(1/sqrt(2.0F)), one_div_root_two, tolerance);

   // pi.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(3.14159265358979323846264338327950288419716939937510F), pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(3.14159265358979323846264338327950288419716939937510F/2), half_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(3.14159265358979323846264338327950288419716939937510F/3), third_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(3.14159265358979323846264338327950288419716939937510F/6), sixth_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(2 * 3.14159265358979323846264338327950288419716939937510F), two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(3 * 3.14159265358979323846264338327950288419716939937510F / 4), three_quarters_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(4 * 3.14159265358979323846264338327950288419716939937510F / 3), four_thirds_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(1 / (2 * 3.14159265358979323846264338327950288419716939937510F)), one_div_two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(3.14159265358979323846264338327950288419716939937510F)), root_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(3.14159265358979323846264338327950288419716939937510F / 2)), root_half_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(2 * 3.14159265358979323846264338327950288419716939937510F)), root_two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(1 / sqrt(3.14159265358979323846264338327950288419716939937510F)), one_div_root_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(1 / sqrt(2 * 3.14159265358979323846264338327950288419716939937510F)), one_div_root_two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(sqrt(1. / 3.14159265358979323846264338327950288419716939937510F)), root_one_div_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(3.14159265358979323846264338327950288419716939937510L - 3.L), pi_minus_three, tolerance * 2 ); // tolerance * 2 because of cancellation loss.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(4.L - 3.14159265358979323846264338327950288419716939937510L), four_minus_pi, tolerance );
   //  BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(pow((4 - 3.14159265358979323846264338327950288419716939937510F), 1.5F)), pow23_four_minus_pi, tolerance); See above.
   //
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(pow((3.14159265358979323846264338327950288419716939937510F), 2.71828182845904523536028747135266249775724709369995F)), pi_pow_e, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(3.14159265358979323846264338327950288419716939937510F * 3.14159265358979323846264338327950288419716939937510F), pi_sqr, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(3.14159265358979323846264338327950288419716939937510F * 3.14159265358979323846264338327950288419716939937510F/6), pi_sqr_div_six, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(3.14159265358979323846264338327950288419716939937510F * 3.14159265358979323846264338327950288419716939937510F * 3.14159265358979323846264338327950288419716939937510F), pi_cubed, tolerance);  // See above.

   // BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(3.14159265358979323846264338327950288419716939937510F * 3.14159265358979323846264338327950288419716939937510F), cbrt_pi, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(cbrt_pi * cbrt_pi * cbrt_pi, pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION((static_cast<float>(1)/cbrt_pi), one_div_cbrt_pi, tolerance);

   // Euler
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(2.71828182845904523536028747135266249775724709369995F), e, tolerance);

   //BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(exp(-0.5F)), exp_minus_half, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(pow(e, pi), e_pow_pi, tolerance); // See also above.
   BOOST_CHECK_CLOSE_FRACTION(sqrt(e), root_e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log10(e), log10_e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<float>(1)/log10(e), one_div_log10_e, tolerance);

   // Trigonmetric
   BOOST_CHECK_CLOSE_FRACTION(pi/180, degree, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(180 / pi, radian, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sin(1.F), sin_one, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(cos(1.F), cos_one, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sinh(1.F), sinh_one, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(cosh(1.F), cosh_one, tolerance);

   // Phi
   BOOST_CHECK_CLOSE_FRACTION((1.F + sqrt(5.F)) /2, phi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log((1.F + sqrt(5.F)) /2), ln_phi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1.F / log((1.F + sqrt(5.F)) /2), one_div_ln_phi, tolerance);

   // Euler's Gamma
   BOOST_CHECK_CLOSE_FRACTION(0.57721566490153286060651209008240243104215933593992F, euler, tolerance); // (sequence A001620 in OEIS).
   BOOST_CHECK_CLOSE_FRACTION(1.F/ 0.57721566490153286060651209008240243104215933593992F, one_div_euler, tolerance); // (from sequence A001620 in OEIS).
   BOOST_CHECK_CLOSE_FRACTION(0.57721566490153286060651209008240243104215933593992F * 0.57721566490153286060651209008240243104215933593992F, euler_sqr, tolerance); // (from sequence A001620 in OEIS).

   // Misc
   BOOST_CHECK_CLOSE_FRACTION(1.644934066848226436472415166646025189218949901206F, zeta_two, tolerance); // A013661 as a constant (usually base 10) in OEIS.
   BOOST_CHECK_CLOSE_FRACTION(1.20205690315959428539973816151144999076498629234049888179227F, zeta_three, tolerance); // (sequence A002117 in OEIS)
   BOOST_CHECK_CLOSE_FRACTION(.91596559417721901505460351493238411077414937428167213F, catalan, tolerance); // A006752 as a constant in OEIS.
   BOOST_CHECK_CLOSE_FRACTION(1.1395470994046486574927930193898461120875997958365518247216557100852480077060706857071875468869385150F, extreme_value_skewness, tolerance); //  Mathematica: N[12 Sqrt[6]  Zeta[3]/Pi^3, 1101]
   BOOST_CHECK_CLOSE_FRACTION(0.6311106578189371381918993515442277798440422031347194976580945856929268196174737254599050270325373067F, rayleigh_skewness, tolerance); // Mathematica: N[2 Sqrt[Pi] (Pi - 3)/((4 - Pi)^(3/2)), 1100]
   BOOST_CHECK_CLOSE_FRACTION(2.450893006876380628486604106197544154e-01F, rayleigh_kurtosis_excess, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(2.68545200106530644530971483548179569382038229399446295305115234555721885953715200280114117493184769799515F, khinchin, tolerance ); // A002210 as a constant https://oeis.org/A002210/constant
   BOOST_CHECK_CLOSE_FRACTION(1.2824271291006226368753425688697917277676889273250011F, glaisher, tolerance ); // https://oeis.org/A074962/constant

} // template <class RealType>void test_spots(RealType)

void test_double_spots()
{
   // Basic sanity checks for constants in boost::math::double_constants::
   // for example: boost::math::double_constants::pi
   // (rather than boost::math::constants::pi<double>() ).

   double tolerance = boost::math::tools::epsilon<double>() * 2;

   using namespace boost::math::double_constants;
   BOOST_MATH_STD_USING

   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(3.14159265358979323846264338327950288419716939937510), pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(3.14159265358979323846264338327950288419716939937510)), root_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(3.14159265358979323846264338327950288419716939937510/2)), root_half_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(3.14159265358979323846264338327950288419716939937510 * 2)), root_two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(log(4.0))), root_ln_four, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(2.71828182845904523536028747135266249775724709369995), e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(0.5), half, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(0.57721566490153286060651209008240243104259335), euler, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(2.0)), root_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(log(2.0)), ln_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(log(log(2.0))), ln_ln_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(1)/3, third, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(2)/3, twothirds, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(0.14159265358979323846264338327950288419716939937510), pi_minus_three, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(4. - 3.14159265358979323846264338327950288419716939937510), four_minus_pi, tolerance);
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(pow((4 - 3.14159265358979323846264338327950288419716939937510), 1.5)), pow23_four_minus_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(pow((3.14159265358979323846264338327950288419716939937510), 2.71828182845904523536028747135266249775724709369995)), pi_pow_e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(pow((3.14159265358979323846264338327950288419716939937510), 0.33333333333333333333333333333333333333333333333333)), cbrt_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(exp(-0.5)), exp_minus_half, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(pow(2.71828182845904523536028747135266249775724709369995, 3.14159265358979323846264338327950288419716939937510)), e_pow_pi, tolerance);


#else // Only double, so no suffix .
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(pow((4 - 3.14159265358979323846264338327950288419716939937510), 1.5)), pow23_four_minus_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(pow((3.14159265358979323846264338327950288419716939937510), 2.71828182845904523536028747135266249775724709369995)), pi_pow_e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(pow((3.14159265358979323846264338327950288419716939937510), 0.33333333333333333333333333333333333333333333333333)), cbrt_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(exp(-0.5)), exp_minus_half, tolerance);
#endif
   // Rational fractions.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(0.333333333333333333333333333333333333333), third, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(0.666666666666666666666666666666666666667), two_thirds, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(0.75), three_quarters, tolerance);
   // Two and related.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(2.)), root_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(3.)), root_three, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(2.)/2), half_root_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(log(2.)), ln_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(log(log(2.0))), ln_ln_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(log(4.0))), root_ln_four, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(1/sqrt(2.0)), one_div_root_two, tolerance);

   // pi.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(3.14159265358979323846264338327950288419716939937510), pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(3.14159265358979323846264338327950288419716939937510/2), half_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(3.14159265358979323846264338327950288419716939937510/3), third_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(3.14159265358979323846264338327950288419716939937510/6), sixth_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(2 * 3.14159265358979323846264338327950288419716939937510), two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(3 * 3.14159265358979323846264338327950288419716939937510 / 4), three_quarters_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(4 * 3.14159265358979323846264338327950288419716939937510 / 3), four_thirds_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(1 / (2 * 3.14159265358979323846264338327950288419716939937510)), one_div_two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(3.14159265358979323846264338327950288419716939937510)), root_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(3.14159265358979323846264338327950288419716939937510 / 2)), root_half_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(2 * 3.14159265358979323846264338327950288419716939937510)), root_two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(1 / sqrt(3.14159265358979323846264338327950288419716939937510)), one_div_root_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(1 / sqrt(2 * 3.14159265358979323846264338327950288419716939937510)), one_div_root_two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(sqrt(1. / 3.14159265358979323846264338327950288419716939937510)), root_one_div_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(3.14159265358979323846264338327950288419716939937510 - 3.), pi_minus_three, tolerance * 2 ); // tolerance * 2 because of cancellation loss.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(4. - 3.14159265358979323846264338327950288419716939937510), four_minus_pi, tolerance );
   //  BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(pow((4 - 3.14159265358979323846264338327950288419716939937510), 1.5)), pow23_four_minus_pi, tolerance); See above.
   //
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(pow((3.14159265358979323846264338327950288419716939937510), 2.71828182845904523536028747135266249775724709369995)), pi_pow_e, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(3.14159265358979323846264338327950288419716939937510 * 3.14159265358979323846264338327950288419716939937510), pi_sqr, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(3.14159265358979323846264338327950288419716939937510 * 3.14159265358979323846264338327950288419716939937510/6), pi_sqr_div_six, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(3.14159265358979323846264338327950288419716939937510 * 3.14159265358979323846264338327950288419716939937510 * 3.14159265358979323846264338327950288419716939937510), pi_cubed, tolerance);  // See above.

   // BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(3.14159265358979323846264338327950288419716939937510 * 3.14159265358979323846264338327950288419716939937510), cbrt_pi, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(cbrt_pi * cbrt_pi * cbrt_pi, pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION((static_cast<double>(1)/cbrt_pi), one_div_cbrt_pi, tolerance);

   // Euler
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(2.71828182845904523536028747135266249775724709369995), e, tolerance);

   //BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(exp(-0.5)), exp_minus_half, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(pow(e, pi), e_pow_pi, tolerance); // See also above.
   BOOST_CHECK_CLOSE_FRACTION(sqrt(e), root_e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log10(e), log10_e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<double>(1)/log10(e), one_div_log10_e, tolerance);

   // Trigonmetric
   BOOST_CHECK_CLOSE_FRACTION(pi/180, degree, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(180 / pi, radian, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sin(1.), sin_one, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(cos(1.), cos_one, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sinh(1.), sinh_one, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(cosh(1.), cosh_one, tolerance);

   // Phi
   BOOST_CHECK_CLOSE_FRACTION((1. + sqrt(5.)) /2, phi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log((1. + sqrt(5.)) /2), ln_phi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1. / log((1. + sqrt(5.)) /2), one_div_ln_phi, tolerance);

   //Euler's Gamma
   BOOST_CHECK_CLOSE_FRACTION(0.57721566490153286060651209008240243104215933593992, euler, tolerance); // (sequence A001620 in OEIS).
   BOOST_CHECK_CLOSE_FRACTION(1./ 0.57721566490153286060651209008240243104215933593992, one_div_euler, tolerance); // (from sequence A001620 in OEIS).
   BOOST_CHECK_CLOSE_FRACTION(0.57721566490153286060651209008240243104215933593992 * 0.57721566490153286060651209008240243104215933593992, euler_sqr, tolerance); // (from sequence A001620 in OEIS).

   // Misc
   BOOST_CHECK_CLOSE_FRACTION(1.644934066848226436472415166646025189218949901206, zeta_two, tolerance); // A013661 as a constant (usually base 10) in OEIS.
   BOOST_CHECK_CLOSE_FRACTION(1.20205690315959428539973816151144999076498629234049888179227, zeta_three, tolerance); // (sequence A002117 in OEIS)
   BOOST_CHECK_CLOSE_FRACTION(.91596559417721901505460351493238411077414937428167213, catalan, tolerance); // A006752 as a constant in OEIS.
   BOOST_CHECK_CLOSE_FRACTION(1.1395470994046486574927930193898461120875997958365518247216557100852480077060706857071875468869385150, extreme_value_skewness, tolerance); //  Mathematica: N[12 Sqrt[6]  Zeta[3]/Pi^3, 1101]
   BOOST_CHECK_CLOSE_FRACTION(0.6311106578189371381918993515442277798440422031347194976580945856929268196174737254599050270325373067, rayleigh_skewness, tolerance); // Mathematica: N[2 Sqrt[Pi] (Pi - 3)/((4 - Pi)^(3/2)), 1100]
   BOOST_CHECK_CLOSE_FRACTION(2.450893006876380628486604106197544154e-01, rayleigh_kurtosis_excess, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(2.68545200106530644530971483548179569382038229399446295305115234555721885953715200280114117493184769799515, khinchin, tolerance ); // A002210 as a constant https://oeis.org/A002210/constant
   BOOST_CHECK_CLOSE_FRACTION(1.2824271291006226368753425688697917277676889273250011, glaisher, tolerance ); // https://oeis.org/A074962/constant

} // template <class RealType>void test_spots(RealType)

void test_long_double_spots()
{
   // Basic sanity checks for constants in boost::math::long double_constants::
   // for example: boost::math::long_double_constants::pi
   // (rather than boost::math::constants::pi<long double>() ).

  // All constants are tested here using at least long double precision
  // with independent calculated or listed values,
  // or calculations using long double (sometime a little less accurate).

   long double tolerance = boost::math::tools::epsilon<long double>() * 2;

   using namespace boost::math::long_double_constants;
   BOOST_MATH_STD_USING

   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(3.14159265358979323846264338327950288419716939937510L), pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(3.14159265358979323846264338327950288419716939937510L)), root_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(3.14159265358979323846264338327950288419716939937510L/2)), root_half_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(3.14159265358979323846264338327950288419716939937510L * 2)), root_two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(log(4.0L))), root_ln_four, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(2.71828182845904523536028747135266249775724709369995L), e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(0.5), half, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(0.57721566490153286060651209008240243104259335L), euler, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(2.0L)), root_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(log(2.0L)), ln_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(log(log(2.0L))), ln_ln_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(1)/3, third, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(2)/3, twothirds, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(0.14159265358979323846264338327950288419716939937510L), pi_minus_three, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(4.L - 3.14159265358979323846264338327950288419716939937510L), four_minus_pi, tolerance);
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(pow((4 - 3.14159265358979323846264338327950288419716939937510L), 1.5L)), pow23_four_minus_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(pow((3.14159265358979323846264338327950288419716939937510L), 2.71828182845904523536028747135266249775724709369995L)), pi_pow_e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(pow((3.14159265358979323846264338327950288419716939937510L), 0.33333333333333333333333333333333333333333333333333L)), cbrt_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(exp(-0.5L)), exp_minus_half, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(pow(2.71828182845904523536028747135266249775724709369995L, 3.14159265358979323846264338327950288419716939937510L)), e_pow_pi, tolerance);


#else // Only double, so no suffix L.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(pow((4 - 3.14159265358979323846264338327950288419716939937510), 1.5)), pow23_four_minus_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(pow((3.14159265358979323846264338327950288419716939937510), 2.71828182845904523536028747135266249775724709369995)), pi_pow_e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(pow((3.14159265358979323846264338327950288419716939937510), 0.33333333333333333333333333333333333333333333333333)), cbrt_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(exp(-0.5)), exp_minus_half, tolerance);
#endif
   // Rational fractions.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(0.333333333333333333333333333333333333333L), third, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(0.666666666666666666666666666666666666667L), two_thirds, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(0.75L), three_quarters, tolerance);
   // Two and related.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(2.L)), root_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(3.L)), root_three, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(2.L)/2), half_root_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(log(2.L)), ln_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(log(log(2.0L))), ln_ln_two, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(log(4.0L))), root_ln_four, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(1/sqrt(2.0L)), one_div_root_two, tolerance);

   // pi.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(3.14159265358979323846264338327950288419716939937510L), pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(3.14159265358979323846264338327950288419716939937510L/2), half_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(3.14159265358979323846264338327950288419716939937510L/3), third_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(3.14159265358979323846264338327950288419716939937510L/6), sixth_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(2 * 3.14159265358979323846264338327950288419716939937510L), two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(3 * 3.14159265358979323846264338327950288419716939937510L / 4), three_quarters_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(4 * 3.14159265358979323846264338327950288419716939937510L / 3), four_thirds_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(1 / (2 * 3.14159265358979323846264338327950288419716939937510L)), one_div_two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(3.14159265358979323846264338327950288419716939937510L)), root_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(3.14159265358979323846264338327950288419716939937510L / 2)), root_half_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(2 * 3.14159265358979323846264338327950288419716939937510L)), root_two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(1 / sqrt(3.14159265358979323846264338327950288419716939937510L)), one_div_root_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(1 / sqrt(2 * 3.14159265358979323846264338327950288419716939937510L)), one_div_root_two_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(sqrt(1. / 3.14159265358979323846264338327950288419716939937510L)), root_one_div_pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(3.14159265358979323846264338327950288419716939937510L - 3.L), pi_minus_three, tolerance * 2 ); // tolerance * 2 because of cancellation loss.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(4.L - 3.14159265358979323846264338327950288419716939937510L), four_minus_pi, tolerance );
   //  BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(pow((4 - 3.14159265358979323846264338327950288419716939937510L), 1.5L)), pow23_four_minus_pi, tolerance); See above.
   //
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(pow((3.14159265358979323846264338327950288419716939937510L), 2.71828182845904523536028747135266249775724709369995L)), pi_pow_e, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L), pi_sqr, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L/6), pi_sqr_div_six, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L), pi_cubed, tolerance);  // See above.

   // BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L), cbrt_pi, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(cbrt_pi * cbrt_pi * cbrt_pi, pi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION((static_cast<long double>(1)/cbrt_pi), one_div_cbrt_pi, tolerance);

   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(6.366197723675813430755350534900574481378385829618257E-1L), two_div_pi, tolerance * 3);  // 2/pi
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(7.97884560802865355879892119868763736951717262329869E-1L), root_two_div_pi, tolerance * 3);  //  sqrt(2/pi)

   // Euler
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(2.71828182845904523536028747135266249775724709369995L), e, tolerance);

   //BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(exp(-0.5L)), exp_minus_half, tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(pow(e, pi), e_pow_pi, tolerance); // See also above.
   BOOST_CHECK_CLOSE_FRACTION(sqrt(e), root_e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log10(e), log10_e, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(1)/log10(e), one_div_log10_e, tolerance);

   // Trigonmetric
   BOOST_CHECK_CLOSE_FRACTION(pi/180, degree, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(180 / pi, radian, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sin(1.L), sin_one, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(cos(1.L), cos_one, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sinh(1.L), sinh_one, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(cosh(1.L), cosh_one, tolerance);

   // Phi
   BOOST_CHECK_CLOSE_FRACTION((1.L + sqrt(5.L)) /2, phi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log((1.L + sqrt(5.L)) /2), ln_phi, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1.L / log((1.L + sqrt(5.L)) /2), one_div_ln_phi, tolerance);

   //Euler's Gamma
   BOOST_CHECK_CLOSE_FRACTION(0.57721566490153286060651209008240243104215933593992L, euler, tolerance); // (sequence A001620 in OEIS).
   BOOST_CHECK_CLOSE_FRACTION(1.L/ 0.57721566490153286060651209008240243104215933593992L, one_div_euler, tolerance); // (from sequence A001620 in OEIS).
   BOOST_CHECK_CLOSE_FRACTION(0.57721566490153286060651209008240243104215933593992L * 0.57721566490153286060651209008240243104215933593992L, euler_sqr, tolerance); // (from sequence A001620 in OEIS).

   // Misc
   BOOST_CHECK_CLOSE_FRACTION(1.644934066848226436472415166646025189218949901206L, zeta_two, tolerance); // A013661 as a constant (usually base 10) in OEIS.
   BOOST_CHECK_CLOSE_FRACTION(1.20205690315959428539973816151144999076498629234049888179227L, zeta_three, tolerance); // (sequence A002117 in OEIS)
   BOOST_CHECK_CLOSE_FRACTION(.91596559417721901505460351493238411077414937428167213L, catalan, tolerance); // A006752 as a constant in OEIS.
   BOOST_CHECK_CLOSE_FRACTION(1.1395470994046486574927930193898461120875997958365518247216557100852480077060706857071875468869385150L, extreme_value_skewness, tolerance); //  Mathematica: N[12 Sqrt[6]  Zeta[3]/Pi^3, 1101]
   BOOST_CHECK_CLOSE_FRACTION(0.6311106578189371381918993515442277798440422031347194976580945856929268196174737254599050270325373067L, rayleigh_skewness, tolerance); // Mathematica: N[2 Sqrt[Pi] (Pi - 3)/((4 - Pi)^(3/2)), 1100]
   BOOST_CHECK_CLOSE_FRACTION(2.450893006876380628486604106197544154e-01L, rayleigh_kurtosis_excess, tolerance);
   BOOST_CHECK_CLOSE_FRACTION(2.68545200106530644530971483548179569382038229399446295305115234555721885953715200280114117493184769799515L, khinchin, tolerance ); // A002210 as a constant https://oeis.org/A002210/constant
   BOOST_CHECK_CLOSE_FRACTION(1.2824271291006226368753425688697917277676889273250011L, glaisher, tolerance ); // https://oeis.org/A074962/constant

} // template <class RealType>void test_spots(RealType)

template <class Policy>
void test_real_concept_policy(const Policy&)
{
   // Basic sanity checks for constants using real_concept.
   // Parameter Policy is used to control precision.

   using boost::math::concepts::real_concept;

   boost::math::concepts::real_concept tolerance = boost::math::tools::epsilon<real_concept>() * 2;  // double
   if(Policy::precision_type::value > 200)
      tolerance *= 50;
   std::cout << "Tolerance for type " << typeid(real_concept).name()  << " is " << tolerance << "." << std::endl;

   //typedef typename boost::math::policies::precision<boost::math::concepts::real_concept, boost::math::policies::policy<> >::type t1;
   // A precision of zero means we don't know what the precision of this type is until runtime.
   //std::cout << "Precision for type " << typeid(boost::math::concepts::real_concept).name()  << " is " << t1::value << "." << std::endl;

   using namespace boost::math::constants;
   BOOST_MATH_STD_USING

   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L, (pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(3.14159265358979323846264338327950288419716939937510L), (root_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(3.14159265358979323846264338327950288419716939937510L/2), (root_half_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(3.14159265358979323846264338327950288419716939937510L * 2), (root_two_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(log(4.0L)), (root_ln_four<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(2.71828182845904523536028747135266249775724709369995L, (e<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(0.5, (half<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(0.57721566490153286060651209008240243104259335L, (euler<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(2.0L), (root_two<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log(2.0L), (ln_two<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log(log(2.0L)), (ln_ln_two<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(1)/3, (third<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(static_cast<long double>(2)/3, (twothirds<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(0.14159265358979323846264338327950288419716939937510L, (pi_minus_three<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(4.L - 3.14159265358979323846264338327950288419716939937510L, (four_minus_pi<real_concept, Policy>)(), tolerance);
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   BOOST_CHECK_CLOSE_FRACTION(pow((4 - 3.14159265358979323846264338327950288419716939937510L), 1.5L), (pow23_four_minus_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(pow((3.14159265358979323846264338327950288419716939937510L), 2.71828182845904523536028747135266249775724709369995L), (pi_pow_e<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(pow((3.14159265358979323846264338327950288419716939937510L), 0.33333333333333333333333333333333333333333333333333L), (cbrt_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(exp(-0.5L), (exp_minus_half<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(pow(2.71828182845904523536028747135266249775724709369995L, 3.14159265358979323846264338327950288419716939937510L), (e_pow_pi<real_concept, Policy>)(), tolerance);


#else // Only double, so no suffix L.
   BOOST_CHECK_CLOSE_FRACTION(pow((4 - 3.14159265358979323846264338327950288419716939937510), 1.5), (pow23_four_minus_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(pow((3.14159265358979323846264338327950288419716939937510), 2.71828182845904523536028747135266249775724709369995), (pi_pow_e<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(pow((3.14159265358979323846264338327950288419716939937510), 0.33333333333333333333333333333333333333333333333333), (cbrt_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(exp(-0.5), (exp_minus_half<real_concept, Policy>)(), tolerance);
#endif
   // Rational fractions.
   BOOST_CHECK_CLOSE_FRACTION(0.333333333333333333333333333333333333333L, (third<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(0.666666666666666666666666666666666666667L, (two_thirds<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(0.75L, (three_quarters<real_concept, Policy>)(), tolerance);
   // Two and related.
   BOOST_CHECK_CLOSE_FRACTION(sqrt(2.L), (root_two<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(3.L), (root_three<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(2.L)/2, (half_root_two<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log(2.L), (ln_two<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log(log(2.0L)), (ln_ln_two<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(log(4.0L)), (root_ln_four<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1/sqrt(2.0L), (one_div_root_two<real_concept, Policy>)(), tolerance);

   // pi.
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L, (pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L/2, (half_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L/3, (third_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L/6, (sixth_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(2 * 3.14159265358979323846264338327950288419716939937510L, (two_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(3 * 3.14159265358979323846264338327950288419716939937510L / 4, (three_quarters_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(4 * 3.14159265358979323846264338327950288419716939937510L / 3, (four_thirds_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1 / (2 * 3.14159265358979323846264338327950288419716939937510L), (one_div_two_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(3.14159265358979323846264338327950288419716939937510L), (root_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(3.14159265358979323846264338327950288419716939937510L / 2), (root_half_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(2 * 3.14159265358979323846264338327950288419716939937510L), (root_two_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1 / sqrt(3.14159265358979323846264338327950288419716939937510L), (one_div_root_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1 / sqrt(2 * 3.14159265358979323846264338327950288419716939937510L), (one_div_root_two_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sqrt(1. / 3.14159265358979323846264338327950288419716939937510L), (root_one_div_pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L - 3.L, (pi_minus_three<real_concept, Policy>)(), tolerance * 2 ); // tolerance * 2 because of cancellation loss.
   BOOST_CHECK_CLOSE_FRACTION(4.L - 3.14159265358979323846264338327950288419716939937510L, (four_minus_pi<real_concept, Policy>)(), tolerance );
   //  BOOST_CHECK_CLOSE_FRACTION(pow((4 - 3.14159265358979323846264338327950288419716939937510L), 1.5L), (pow23_four_minus_pi<real_concept, Policy>)(), tolerance); See above.
   //
   BOOST_CHECK_CLOSE_FRACTION(pow((3.14159265358979323846264338327950288419716939937510L), 2.71828182845904523536028747135266249775724709369995L), (pi_pow_e<real_concept, Policy>)(), tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L, (pi_sqr<real_concept, Policy>)(), tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L/6, (pi_sqr_div_six<real_concept, Policy>)(), tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L, (pi_cubed<real_concept, Policy>)(), tolerance);  // See above.

   // BOOST_CHECK_CLOSE_FRACTION(3.14159265358979323846264338327950288419716939937510L * 3.14159265358979323846264338327950288419716939937510L, (cbrt_pi<real_concept, Policy>)(), tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION((cbrt_pi<real_concept, Policy>)() * (cbrt_pi<real_concept, Policy>)() * (cbrt_pi<real_concept, Policy>)(), (pi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION((1)/(cbrt_pi<real_concept, Policy>)(), (one_div_cbrt_pi<real_concept, Policy>)(), tolerance);

   // Euler
   BOOST_CHECK_CLOSE_FRACTION(2.71828182845904523536028747135266249775724709369995L, (e<real_concept, Policy>)(), tolerance);

   //BOOST_CHECK_CLOSE_FRACTION(exp(-0.5L), (exp_minus_half<real_concept, Policy>)(), tolerance);  // See above.
   BOOST_CHECK_CLOSE_FRACTION(pow(e<real_concept, Policy>(), (pi<real_concept, Policy>)()), (e_pow_pi<real_concept, Policy>)(), tolerance); // See also above.
   BOOST_CHECK_CLOSE_FRACTION(sqrt(e<real_concept, Policy>()), (root_e<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log10(e<real_concept, Policy>()), (log10_e<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1/log10(e<real_concept, Policy>()), (one_div_log10_e<real_concept, Policy>)(), tolerance);

   // Trigonmetric
   BOOST_CHECK_CLOSE_FRACTION((pi<real_concept, Policy>)()/180, (degree<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(180 / (pi<real_concept, Policy>)(), (radian<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sin(1.L), (sin_one<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(cos(1.L), (cos_one<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(sinh(1.L), (sinh_one<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(cosh(1.L), (cosh_one<real_concept, Policy>)(), tolerance);

   // Phi
   BOOST_CHECK_CLOSE_FRACTION((1.L + sqrt(5.L)) /2, (phi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(log((1.L + sqrt(5.L)) /2), (ln_phi<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(1.L / log((1.L + sqrt(5.L)) /2), (one_div_ln_phi<real_concept, Policy>)(), tolerance);

   //Euler's Gamma
   BOOST_CHECK_CLOSE_FRACTION(0.57721566490153286060651209008240243104215933593992L, (euler<real_concept, Policy>)(), tolerance); // (sequence A001620 in OEIS).
   BOOST_CHECK_CLOSE_FRACTION(1.L/ 0.57721566490153286060651209008240243104215933593992L, (one_div_euler<real_concept, Policy>)(), tolerance); // (from sequence A001620 in OEIS).
   BOOST_CHECK_CLOSE_FRACTION(0.57721566490153286060651209008240243104215933593992L * 0.57721566490153286060651209008240243104215933593992L, (euler_sqr<real_concept, Policy>)(), tolerance); // (from sequence A001620 in OEIS).

   // Misc
   BOOST_CHECK_CLOSE_FRACTION(1.644934066848226436472415166646025189218949901206L, (zeta_two<real_concept, Policy>)(), tolerance); // A013661 as a constant (usually base 10) in OEIS.
   BOOST_CHECK_CLOSE_FRACTION(1.20205690315959428539973816151144999076498629234049888179227L, (zeta_three<real_concept, Policy>)(), tolerance); // (sequence A002117 in OEIS)
   BOOST_CHECK_CLOSE_FRACTION(.91596559417721901505460351493238411077414937428167213L, (catalan<real_concept, Policy>)(), tolerance); // A006752 as a constant in OEIS.
   BOOST_CHECK_CLOSE_FRACTION(1.1395470994046486574927930193898461120875997958365518247216557100852480077060706857071875468869385150L, (extreme_value_skewness<real_concept, Policy>)(), tolerance); //  Mathematica: N[12 Sqrt[6]  Zeta[3]/Pi^3, 1101]
   BOOST_CHECK_CLOSE_FRACTION(0.6311106578189371381918993515442277798440422031347194976580945856929268196174737254599050270325373067L, (rayleigh_skewness<real_concept, Policy>)(), tolerance); // Mathematica: N[2 Sqrt[Pi] (Pi - 3)/((4 - Pi)^(3/2)), 1100]
   BOOST_CHECK_CLOSE_FRACTION(2.450893006876380628486604106197544154e-01L, (rayleigh_kurtosis_excess<real_concept, Policy>)(), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(2.68545200106530644530971483548179569382038229399446295305115234555721885953715200280114117493184769799515L, (khinchin<real_concept, Policy>)(), tolerance ); // A002210 as a constant https://oeis.org/A002210/constant
   BOOST_CHECK_CLOSE_FRACTION(1.2824271291006226368753425688697917277676889273250011L, (glaisher<real_concept, Policy>)(), tolerance ); // https://oeis.org/A074962/constant

   //
   // Last of all come the test cases that behave differently if we're calculating the constants on the fly:
   //
   if(boost::math::tools::digits<real_concept>() > boost::math::constants::max_string_digits)
   {
      // This suffers from cancellation error, so increased tolerance:
      BOOST_CHECK_CLOSE_FRACTION((static_cast<real_concept>(4. - 3.14159265358979323846264338327950288419716939937510L)), (four_minus_pi<real_concept, Policy>)(), tolerance * 3);
      BOOST_CHECK_CLOSE_FRACTION((static_cast<real_concept>(0.14159265358979323846264338327950288419716939937510L)), (pi_minus_three<real_concept, Policy>)(), tolerance * 3);
   }
   else
   {
      BOOST_CHECK_CLOSE_FRACTION((static_cast<real_concept>(4. - 3.14159265358979323846264338327950288419716939937510L)), (four_minus_pi<real_concept, Policy>)(), tolerance);
      BOOST_CHECK_CLOSE_FRACTION((static_cast<real_concept>(0.14159265358979323846264338327950288419716939937510L)), (pi_minus_three<real_concept, Policy>)(), tolerance);
   }

} // template <class boost::math::concepts::real_concept>void test_spots(boost::math::concepts::real_concept)

#ifdef BOOST_MATH_USE_FLOAT128
void test_float128()
{
   static const __float128 eps = 1.92592994438723585305597794258492732e-34Q;

   __float128 p = boost::math::constants::pi<__float128>();
   __float128 r = 3.14159265358979323846264338327950288419716939937510582097494459230781640628620899862803482534211706798214808651Q;
   __float128 err = (p - r) / r;
   if(err < 0)
      err = -err;
   BOOST_CHECK(err < 2 * eps);
}
#endif

void test_constexpr()
{
#ifndef BOOST_NO_CXX11_CONSTEXPR
   constexpr float f1 = boost::math::constants::pi<float>();
   constexpr double f2 = boost::math::constants::pi<double>();
   constexpr long double f3 = boost::math::constants::pi<long double>();
   (void)f1;
   (void)f2;
   (void)f3;
#ifdef BOOST_MATH_USE_FLOAT128
   constexpr __float128 f4 = boost::math::constants::pi<__float128>();
   (void)f4;
#endif
#endif
}

BOOST_AUTO_TEST_CASE( test_main )
{
   // Basic sanity-check spot values.

   test_float_spots(); // Test float_constants, like boost::math::float_constants::pi;
   test_double_spots(); // Test double_constants.
   test_long_double_spots(); // Test long_double_constants.
#ifdef BOOST_MATH_USE_FLOAT128
   test_float128();
#endif
   test_constexpr();

   test_real_concept_policy(real_concept_policy_1());
   test_real_concept_policy(real_concept_policy_2()); // Increased precision forcing construction from string.
   test_real_concept_policy(real_concept_policy_3()); // Increased precision forcing caching of computed values.
   test_real_concept_policy(boost::math::policies::policy<>()); // Default.

   // (Parameter value, arbitrarily zero, only communicates the floating-point type).
   test_spots(0.0F); // Test float.
   test_spots(0.0); // Test double.
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   test_spots(0.0L); // Test long double.
#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x0582))
   test_spots(boost::math::concepts::real_concept(0.)); // Test real concept.
   test_spots(boost::math::concepts::big_real_concept(0.)); // Test real concept.
#endif
#else
  std::cout << "<note>The long double tests have been disabled on this platform "
    "either because the long double overloads of the usual math functions are "
    "not available at all, or because they are too inaccurate for these tests "
    "to pass.</note>" << std::cout;
#endif

} // BOOST_AUTO_TEST_CASE( test_main )

/*

Output:

  1 Feb 2012

test_constants.cpp
  test_constants.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Debug\test_constants.exe
  Running 1 test case...
  Tolerance for type class boost::math::concepts::real_concept is 4.44089e-016.
  Tolerance for type class boost::math::concepts::real_concept is 4.44089e-016.
  Tolerance for type class boost::math::concepts::real_concept is 4.44089e-016.
  Tolerance for type float is 2.38419e-007.
  Tolerance for type double is 4.44089e-016.
  Tolerance for type long double is 4.44089e-016.
  Tolerance for type class boost::math::concepts::real_concept is 4.44089e-016.
  Tolerance for type class boost::math::concepts::big_real_concept is 1.33227e-014.

  *** No errors detected

*/

