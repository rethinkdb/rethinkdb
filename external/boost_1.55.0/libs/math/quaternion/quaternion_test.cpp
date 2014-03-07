// test file for quaternion.hpp

//  (C) Copyright Hubert Holin 2001.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)


#include <iomanip>


#include <boost/mpl/list.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>


#include <boost/math/quaternion.hpp>

template<typename T>
struct string_type_name;

#define DEFINE_TYPE_NAME(Type)              \
template<> struct string_type_name<Type>    \
{                                           \
    static char const * _()                 \
    {                                       \
        return #Type;                       \
    }                                       \
}

DEFINE_TYPE_NAME(float);
DEFINE_TYPE_NAME(double);
DEFINE_TYPE_NAME(long double);


#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
typedef boost::mpl::list<float,double,long double>  test_types;
#else
typedef boost::mpl::list<float,double>  test_types;
#endif

// Apple GCC 4.0 uses the "double double" format for its long double,
// which means that epsilon is VERY small but useless for
// comparisons. So, don't do those comparisons.
#if (defined(__APPLE_CC__) && defined(__GNUC__) && __GNUC__ == 4) || defined(BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS)
typedef boost::mpl::list<float,double>  near_eps_test_types;
#else
typedef boost::mpl::list<float,double,long double>  near_eps_test_types;
#endif


#if BOOST_WORKAROUND(__GNUC__, < 3)
    // gcc 2.x ignores function scope using declarations,
    // put them in the scope of the enclosing namespace instead:
using   ::std::sqrt;
using   ::std::atan;
using   ::std::log;
using   ::std::exp;
using   ::std::cos;
using   ::std::sin;
using   ::std::tan;
using   ::std::cosh;
using   ::std::sinh;
using   ::std::tanh;

using   ::std::numeric_limits;

using   ::boost::math::abs;
#endif  /* BOOST_WORKAROUND(__GNUC__, < 3) */

#ifdef  BOOST_NO_STDC_NAMESPACE
using   ::sqrt;
using   ::atan;
using   ::log;
using   ::exp;
using   ::cos;
using   ::sin;
using   ::tan;
using   ::cosh;
using   ::sinh;
using   ::tanh;
#endif  /* BOOST_NO_STDC_NAMESPACE */

#ifdef  BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
using   ::boost::math::real;
using   ::boost::math::unreal;
using   ::boost::math::sup;
using   ::boost::math::l1;
using   ::boost::math::abs;
using   ::boost::math::norm;
using   ::boost::math::conj;
using   ::boost::math::exp;
using   ::boost::math::pow;
using   ::boost::math::cos;
using   ::boost::math::sin;
using   ::boost::math::tan;
using   ::boost::math::cosh;
using   ::boost::math::sinh;
using   ::boost::math::tanh;
using   ::boost::math::sinc_pi;
using   ::boost::math::sinhc_pi;
#endif  /* BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP */
  
// Provide standard floating point abs() overloads if older Microsoft
// library is used with _MSC_EXTENSIONS defined. This code also works
// for the Intel compiler using the Microsoft library.
#if defined(_MSC_EXTENSIONS) && BOOST_WORKAROUND(_MSC_VER, < 1310)
#if !((__INTEL__ && _WIN32) && BOOST_WORKAROUND(__MWERKS__, >= 0x3201))
inline float        abs(float v)
{
    return(fabs(v));
}

inline double        abs(double v)
{
    return(fabs(v));
}

inline long double    abs(long double v)
{
    return(fabs(v));
}
#endif /* !((__INTEL__ && _WIN32) && BOOST_WORKAROUND(__MWERKS__, >= 0x3201)) */
#endif /* defined(_MSC_EXTENSIONS) && BOOST_WORKAROUND(_MSC_VER, < 1310) */


// explicit (if ludicrous) instanciation
#if !BOOST_WORKAROUND(__GNUC__, < 3)
template    class ::boost::math::quaternion<int>;
#else
// gcc doesn't like the absolutely-qualified namespace
template class boost::math::quaternion<int>;
#endif /* !BOOST_WORKAROUND(__GNUC__) */



void    quaternion_manual_test()
{
    // tests for evaluation by humans
    
    
    // using default constructor
    ::boost::math::quaternion<float>        q0;
    
    ::boost::math::quaternion<float>        qa[2];
    
    // using constructor "H seen as R^4"
    ::boost::math::quaternion<double>       q1(1,2,3,4);
    
    ::std::complex<float>                   c0(5,6);
    
    // using constructor "H seen as C^2"
    ::boost::math::quaternion<float>        q2(c0);
    
    // using UNtemplated copy constructor
    ::boost::math::quaternion<float>        q3(q2);
    
    // using templated copy constructor
    ::boost::math::quaternion<long double>  q4(q3);
    
    // using UNtemplated assignment operator
    q3 = q0;
    qa[0] = q0;
    
    // using templated assignment operator
    q4 = q0;
    qa[1] = q1;
    
    float                                   f0(7);
    
    // using converting assignment operator
    q2 = f0;
    
    // using converting assignment operator
    q3 = c0;
    
    // using += (const T &)
    q2 += f0;
    
    // using += (const ::std::complex<T> &)
    q2 += c0;
    
    // using += (const quaternion<X> &)
    q2 += q3;
    
    // using -= (const T &)
    q3 -= f0;
    
    // using -= (const ::std::complex<T> &)
    q3 -= c0;
    
    // using -= (const quaternion<X> &)
    q3 -= q2;
    
    double                                  d0(8);
    ::std::complex<double>                  c1(9,10);
    
    // using *= (const T &)
    q1 *= d0;
    
    // using *= (const ::std::complex<T> &)
    q1 *= c1;
    
    // using *= (const quaternion<X> &)
    q1 *= q1;
    
    long double                             l0(11);
    ::std::complex<long double>             c2(12,13);
    
    // using /= (const T &)
    q4 /= l0;
    
    // using /= (const ::std::complex<T> &)
    q4 /= c2;
    
    // using /= (const quaternion<X> &)
    q4 /= q1;
    
    // using + (const T &, const quaternion<T> &)
    ::boost::math::quaternion<float>        q5 = f0+q2;
    
    // using + (const quaternion<T> &, const T &)
    ::boost::math::quaternion<float>        q6 = q2+f0;
    
    // using + (const ::std::complex<T> &, const quaternion<T> &)
    ::boost::math::quaternion<float>        q7 = c0+q2;
    
    // using + (const quaternion<T> &, const ::std::complex<T> &)
    ::boost::math::quaternion<float>        q8 = q2+c0;
    
    // using + (const quaternion<T> &,const quaternion<T> &)
    ::boost::math::quaternion<float>        q9 = q2+q3;
    
    // using - (const T &, const quaternion<T> &)
    q5 = f0-q2;
    
    // using - (const quaternion<T> &, const T &)
    q6 = q2-f0;
    
    // using - (const ::std::complex<T> &, const quaternion<T> &)
    q7 = c0-q2;
    
    // using - (const quaternion<T> &, const ::std::complex<T> &)
    q8 = q2-c0;
    
    // using - (const quaternion<T> &,const quaternion<T> &)
    q9 = q2-q3;
    
    // using * (const T &, const quaternion<T> &)
    q5 = f0*q2;
    
    // using * (const quaternion<T> &, const T &)
    q6 = q2*f0;
    
    // using * (const ::std::complex<T> &, const quaternion<T> &)
    q7 = c0*q2;
    
    // using * (const quaternion<T> &, const ::std::complex<T> &)
    q8 = q2*c0;
    
    // using * (const quaternion<T> &,const quaternion<T> &)
    q9 = q2*q3;
    
    // using / (const T &, const quaternion<T> &)
    q5 = f0/q2;
    
    // using / (const quaternion<T> &, const T &)
    q6 = q2/f0;
    
    // using / (const ::std::complex<T> &, const quaternion<T> &)
    q7 = c0/q2;
    
    // using / (const quaternion<T> &, const ::std::complex<T> &)
    q8 = q2/c0;
    
    // using / (const quaternion<T> &,const quaternion<T> &)
    q9 = q2/q3;
    
    // using + (const quaternion<T> &)
    q2 = +q0;
    
    // using - (const quaternion<T> &)
    q2 = -q3;
    
    // using == (const T &, const quaternion<T> &)
    f0 == q2;
    
    // using == (const quaternion<T> &, const T &)
    q2 == f0;
    
    // using == (const ::std::complex<T> &, const quaternion<T> &)
    c0 == q2;
    
    // using == (const quaternion<T> &, const ::std::complex<T> &)
    q2 == c0;
    
    // using == (const quaternion<T> &,const quaternion<T> &)
    q2 == q3;
    
    // using != (const T &, const quaternion<T> &)
    f0 != q2;
    
    // using != (const quaternion<T> &, const T &)
    q2 != f0;
    
    // using != (const ::std::complex<T> &, const quaternion<T> &)
    c0 != q2;
    
    // using != (const quaternion<T> &, const ::std::complex<T> &)
    q2 != c0;
    
    // using != (const quaternion<T> &,const quaternion<T> &)
    q2 != q3;
    
    BOOST_TEST_MESSAGE("Please input a quaternion...");
    
#ifdef BOOST_INTERACTIVE_TEST_INPUT_ITERATOR
    ::std::cin >> q0;
    
    if    (::std::cin.fail())
    {
        BOOST_TEST_MESSAGE("You have entered nonsense!");
    }
    else
    {
        BOOST_TEST_MESSAGE("You have entered the quaternion "<< q0 << " .");
    }
#else
    ::std::istringstream                bogus("(1,2,3,4)");
    
    bogus >> q0;
    
    BOOST_TEST_MESSAGE("You have entered the quaternion " << q0 << " .");
#endif
    
    BOOST_TEST_MESSAGE("For this quaternion:");
    
    BOOST_TEST_MESSAGE( "the value of the real part is "
                << real(q0));
    
    BOOST_TEST_MESSAGE( "the value of the unreal part is "
                << unreal(q0));
    
    BOOST_TEST_MESSAGE( "the value of the sup norm is "
                << sup(q0));
    
    BOOST_TEST_MESSAGE( "the value of the l1 norm is "
                << l1(q0));
    
    BOOST_TEST_MESSAGE( "the value of the magnitude (euclidian norm) is "
                << abs(q0));
    
    BOOST_TEST_MESSAGE( "the value of the (Cayley) norm is "
                << norm(q0));
    
    BOOST_TEST_MESSAGE( "the value of the conjugate is "
                << conj(q0));
    
    BOOST_TEST_MESSAGE( "the value of the exponential is "
                << exp(q0));
    
    BOOST_TEST_MESSAGE( "the value of the cube is "
                << pow(q0,3));
    
    BOOST_TEST_MESSAGE( "the value of the cosinus is "
                << cos(q0));
    
    BOOST_TEST_MESSAGE( "the value of the sinus is "
                << sin(q0));
    
    BOOST_TEST_MESSAGE( "the value of the tangent is "
                << tan(q0));
    
    BOOST_TEST_MESSAGE( "the value of the hyperbolic cosinus is "
                << cosh(q0));
    
    BOOST_TEST_MESSAGE( "the value of the hyperbolic sinus is "
                << sinh(q0));
    
    BOOST_TEST_MESSAGE( "the value of the hyperbolic tangent is "
                << tanh(q0));
    
#ifdef    BOOST_NO_TEMPLATE_TEMPLATES
    BOOST_TEST_MESSAGE("no template templates, can't compute cardinal functions");
#else    /* BOOST_NO_TEMPLATE_TEMPLATES */
    BOOST_TEST_MESSAGE( "the value of "
                << "the Sinus Cardinal (of index pi) is "
                << sinc_pi(q0));
    
    BOOST_TEST_MESSAGE( "the value of "
                << "the Hyperbolic Sinus Cardinal (of index pi) is "
                << sinhc_pi(q0));
#endif    /* BOOST_NO_TEMPLATE_TEMPLATES */
    
    BOOST_TEST_MESSAGE(" ");
    
    float                         rho = ::std::sqrt(8.0f);
    float                         theta = ::std::atan(1.0f);
    float                         phi1 = ::std::atan(1.0f);
    float                         phi2 = ::std::atan(1.0f);
    
    BOOST_TEST_MESSAGE( "The value of the quaternion represented "
                << "in spherical form by "
                << "rho = " << rho << " , theta = " << theta
                << " , phi1 = " << phi1 << " , phi2 = " << phi2
                << " is "
                << ::boost::math::spherical(rho, theta, phi1, phi2));
    
    float                         alpha = ::std::atan(1.0f);
    
    BOOST_TEST_MESSAGE( "The value of the quaternion represented "
                << "in semipolar form by "
                << "rho = " << rho << " , alpha = " << alpha
                << " , phi1 = " << phi1 << " , phi2 = " << phi2
                << " is "
                << ::boost::math::semipolar(rho, alpha, phi1, phi2));
    
    float                         rho1 = 1;
    float                         rho2 = 2;
    float                         theta1 = 0;
    float                         theta2 = ::std::atan(1.0f)*2;
    
    BOOST_TEST_MESSAGE( "The value of the quaternion represented "
                << "in multipolar form by "
                << "rho1 = " << rho1 << " , theta1 = " << theta1
                << " , rho2 = " << rho2 << " , theta2 = " << theta2
                << " is "
                << ::boost::math::multipolar(rho1, theta1, rho2, theta2));
    
    float                         t = 5;
    float                         radius = ::std::sqrt(2.0f);
    float                         longitude = ::std::atan(1.0f);
    float                         lattitude = ::std::atan(::std::sqrt(3.0f));
    
    BOOST_TEST_MESSAGE( "The value of the quaternion represented "
                << "in cylindrospherical form by "
                << "t = " << t << " , radius = " << radius
                << " , longitude = " << longitude << " , latitude = "
                << lattitude << " is "
                << ::boost::math::cylindrospherical(t, radius,
                        longitude, lattitude));
    
    float                         r = ::std::sqrt(2.0f);
    float                         angle = ::std::atan(1.0f);
    float                         h1 = 3;
    float                         h2 = 4;
    
    BOOST_TEST_MESSAGE( "The value of the quaternion represented "
                << "in cylindrical form by "
                << "r = " << r << " , angle = " << angle
                << " , h1 = " << h1 << " , h2 = " << h2
                << " is "
                << ::boost::math::cylindrical(r, angle, h1, h2));
    
    double                                   real_1(1);
    ::std::complex<double>                   complex_1(1);
    ::std::complex<double>                   complex_i(0,1);
    ::boost::math::quaternion<double>        quaternion_1(1);
    ::boost::math::quaternion<double>        quaternion_i(0,1);
    ::boost::math::quaternion<double>        quaternion_j(0,0,1);
    ::boost::math::quaternion<double>        quaternion_k(0,0,0,1);
    
    BOOST_TEST_MESSAGE(" ");
    
    BOOST_TEST_MESSAGE( "Real 1: " << real_1 << " ; "
                << "Complex 1: " << complex_1 << " ; "
                << "Quaternion 1: " << quaternion_1 << " .");
    
    BOOST_TEST_MESSAGE( "Complex i: " << complex_i << " ; "
                << "Quaternion i: " << quaternion_i << " .");
    
    BOOST_TEST_MESSAGE( "Quaternion j: " << quaternion_j << " .");
    
    BOOST_TEST_MESSAGE( "Quaternion k: " << quaternion_k << " .");
    
    
    BOOST_TEST_MESSAGE(" ");
    
    
    BOOST_TEST_MESSAGE( "i*i: " << quaternion_i*quaternion_i << " ; "
                << "j*j: " << quaternion_j*quaternion_j << " ; "
                << "k*k: " << quaternion_k*quaternion_k << " .");
    BOOST_TEST_MESSAGE( "i*j: " << quaternion_i*quaternion_j << " ; "
                << "j*i: " << quaternion_j*quaternion_i << " .");
    BOOST_TEST_MESSAGE( "j*k: " << quaternion_j*quaternion_k << " ; "
                << "k*j: " << quaternion_k*quaternion_j << " .");
    BOOST_TEST_MESSAGE( "k*i: " << quaternion_k*quaternion_i << " ; "
                << "i*k: " << quaternion_i*quaternion_k << " .");
    
    BOOST_TEST_MESSAGE(" ");
}


BOOST_TEST_CASE_TEMPLATE_FUNCTION(multiplication_test, T)
{
#if     BOOST_WORKAROUND(__GNUC__, < 3)
#else   /* BOOST_WORKAROUND(__GNUC__, < 3) */
    using ::std::numeric_limits;
    
    using ::boost::math::abs;
#endif  /* BOOST_WORKAROUND(__GNUC__, < 3) */
    
    
    BOOST_TEST_MESSAGE("Testing multiplication for "
        << string_type_name<T>::_() << ".");
    
    BOOST_REQUIRE_PREDICATE(::std::less_equal<T>(),
        (abs(::boost::math::quaternion<T>(1,0,0,0)*
             ::boost::math::quaternion<T>(1,0,0,0)-static_cast<T>(1)))
        (numeric_limits<T>::epsilon()));
    
    BOOST_REQUIRE_PREDICATE(::std::less_equal<T>(),
        (abs(::boost::math::quaternion<T>(0,1,0,0)*
             ::boost::math::quaternion<T>(0,1,0,0)+static_cast<T>(1)))
        (numeric_limits<T>::epsilon()));
    
    BOOST_REQUIRE_PREDICATE(::std::less_equal<T>(),
        (abs(::boost::math::quaternion<T>(0,0,1,0)*
             ::boost::math::quaternion<T>(0,0,1,0)+static_cast<T>(1)))
        (numeric_limits<T>::epsilon()));
    
    BOOST_REQUIRE_PREDICATE(::std::less_equal<T>(),
        (abs(::boost::math::quaternion<T>(0,0,0,1)*
             ::boost::math::quaternion<T>(0,0,0,1)+static_cast<T>(1)))
        (numeric_limits<T>::epsilon()));
    
    BOOST_REQUIRE_PREDICATE(::std::less_equal<T>(),
        (abs(::boost::math::quaternion<T>(0,1,0,0)*
             ::boost::math::quaternion<T>(0,0,1,0)-
             ::boost::math::quaternion<T>(0,0,0,1)))
        (numeric_limits<T>::epsilon()));
    
    BOOST_REQUIRE_PREDICATE(::std::less_equal<T>(),
        (abs(::boost::math::quaternion<T>(0,0,1,0)*
             ::boost::math::quaternion<T>(0,1,0,0)+
             ::boost::math::quaternion<T>(0,0,0,1)))
        (numeric_limits<T>::epsilon()));
    
    BOOST_REQUIRE_PREDICATE(::std::less_equal<T>(),
        (abs(::boost::math::quaternion<T>(0,0,1,0)*
             ::boost::math::quaternion<T>(0,0,0,1)-
             ::boost::math::quaternion<T>(0,1,0,0)))
        (numeric_limits<T>::epsilon()));
    
    BOOST_REQUIRE_PREDICATE(::std::less_equal<T>(),
        (abs(::boost::math::quaternion<T>(0,0,0,1)*
             ::boost::math::quaternion<T>(0,0,1,0)+
             ::boost::math::quaternion<T>(0,1,0,0)))
        (numeric_limits<T>::epsilon()));
    
    BOOST_REQUIRE_PREDICATE(::std::less_equal<T>(),
        (abs(::boost::math::quaternion<T>(0,0,0,1)*
             ::boost::math::quaternion<T>(0,1,0,0)-
             ::boost::math::quaternion<T>(0,0,1,0)))
        (numeric_limits<T>::epsilon()));
    
    BOOST_REQUIRE_PREDICATE(::std::less_equal<T>(),
        (abs(::boost::math::quaternion<T>(0,1,0,0)*
             ::boost::math::quaternion<T>(0,0,0,1)+
             ::boost::math::quaternion<T>(0,0,1,0)))
        (numeric_limits<T>::epsilon()));
}


BOOST_TEST_CASE_TEMPLATE_FUNCTION(exp_test, T)
{
#if     BOOST_WORKAROUND(__GNUC__, < 3)
#else   /* BOOST_WORKAROUND(__GNUC__, < 3) */
    using ::std::numeric_limits;
    
    using ::std::atan;
    
    using ::boost::math::abs;
#endif  /* BOOST_WORKAROUND(__GNUC__, < 3) */
    
    
    BOOST_TEST_MESSAGE("Testing exp for "
        << string_type_name<T>::_() << ".");
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(exp(::boost::math::quaternion<T>
             (0,4*atan(static_cast<T>(1)),0,0))+static_cast<T>(1)))
        (2*numeric_limits<T>::epsilon()));
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(exp(::boost::math::quaternion<T>
             (0,0,4*atan(static_cast<T>(1)),0))+static_cast<T>(1)))
        (2*numeric_limits<T>::epsilon()));
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(exp(::boost::math::quaternion<T>
             (0,0,0,4*atan(static_cast<T>(1))))+static_cast<T>(1)))
        (2*numeric_limits<T>::epsilon()));
}


BOOST_TEST_CASE_TEMPLATE_FUNCTION(cos_test, T)
{
#if     BOOST_WORKAROUND(__GNUC__, < 3)
#else   /* BOOST_WORKAROUND(__GNUC__, < 3) */
    using ::std::numeric_limits;
    
    using ::std::log;
    
    using ::boost::math::abs;
#endif  /* BOOST_WORKAROUND(__GNUC__, < 3) */
    
    
    BOOST_TEST_MESSAGE("Testing cos for "
        << string_type_name<T>::_() << ".");
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(static_cast<T>(4)*cos(::boost::math::quaternion<T>
             (0,log(static_cast<T>(2)),0,0))-static_cast<T>(5)))
        (4*numeric_limits<T>::epsilon()));
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(static_cast<T>(4)*cos(::boost::math::quaternion<T>
             (0,0,log(static_cast<T>(2)),0))-static_cast<T>(5)))
        (4*numeric_limits<T>::epsilon()));
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(static_cast<T>(4)*cos(::boost::math::quaternion<T>
             (0,0,0,log(static_cast<T>(2))))-static_cast<T>(5)))
        (4*numeric_limits<T>::epsilon()));
}


BOOST_TEST_CASE_TEMPLATE_FUNCTION(sin_test, T)
{
#if     BOOST_WORKAROUND(__GNUC__, < 3)
#else   /* BOOST_WORKAROUND(__GNUC__, < 3) */
    using ::std::numeric_limits;
    
    using ::std::log;
    
    using ::boost::math::abs;
#endif  /* BOOST_WORKAROUND(__GNUC__, < 3) */
    
    
    BOOST_TEST_MESSAGE("Testing sin for "
        << string_type_name<T>::_() << ".");
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(static_cast<T>(4)*sin(::boost::math::quaternion<T>
             (0,log(static_cast<T>(2)),0,0))
             -::boost::math::quaternion<T>(0,3,0,0)))
        (4*numeric_limits<T>::epsilon()));
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(static_cast<T>(4)*sin(::boost::math::quaternion<T>
             (0,0,log(static_cast<T>(2)),0))
             -::boost::math::quaternion<T>(0,0,3,0)))
        (4*numeric_limits<T>::epsilon()));
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(static_cast<T>(4)*sin(::boost::math::quaternion<T>
             (0,0,0,log(static_cast<T>(2))))
             -::boost::math::quaternion<T>(0,0,0,3)))
        (4*numeric_limits<T>::epsilon()));
}


BOOST_TEST_CASE_TEMPLATE_FUNCTION(cosh_test, T)
{
#if     BOOST_WORKAROUND(__GNUC__, < 3)
#else   /* BOOST_WORKAROUND(__GNUC__, < 3) */
    using ::std::numeric_limits;
    
    using ::std::atan;
    
    using ::boost::math::abs;
#endif  /* BOOST_WORKAROUND(__GNUC__, < 3) */
    
    
    BOOST_TEST_MESSAGE("Testing cosh for "
        << string_type_name<T>::_() << ".");
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(cosh(::boost::math::quaternion<T>
             (0,4*atan(static_cast<T>(1)),0,0))
             +static_cast<T>(1)))
        (4*numeric_limits<T>::epsilon()));
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(cosh(::boost::math::quaternion<T>
             (0,0,4*atan(static_cast<T>(1)),0))
             +static_cast<T>(1)))
        (4*numeric_limits<T>::epsilon()));
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(cosh(::boost::math::quaternion<T>
             (0,0,0,4*atan(static_cast<T>(1))))
             +static_cast<T>(1)))
        (4*numeric_limits<T>::epsilon()));
}


BOOST_TEST_CASE_TEMPLATE_FUNCTION(sinh_test, T)
{
#if     BOOST_WORKAROUND(__GNUC__, < 3)
#else   /* BOOST_WORKAROUND(__GNUC__, < 3) */
    using ::std::numeric_limits;
    
    using ::std::atan;
    
    using ::boost::math::abs;
#endif  /* BOOST_WORKAROUND(__GNUC__, < 3) */
    
    
    BOOST_TEST_MESSAGE("Testing sinh for "
        << string_type_name<T>::_() << ".");
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(sinh(::boost::math::quaternion<T>
             (0,2*atan(static_cast<T>(1)),0,0))
             -::boost::math::quaternion<T>(0,1,0,0)))
        (4*numeric_limits<T>::epsilon()));
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(sinh(::boost::math::quaternion<T>
             (0,0,2*atan(static_cast<T>(1)),0))
             -::boost::math::quaternion<T>(0,0,1,0)))
        (4*numeric_limits<T>::epsilon()));
    
    BOOST_CHECK_PREDICATE(::std::less_equal<T>(),
        (abs(sinh(::boost::math::quaternion<T>
             (0,0,0,2*atan(static_cast<T>(1))))
             -::boost::math::quaternion<T>(0,0,0,1)))
        (4*numeric_limits<T>::epsilon()));
}


boost::unit_test::test_suite *    init_unit_test_suite(int, char *[])
{
    ::boost::unit_test::unit_test_log.
        set_threshold_level(::boost::unit_test::log_messages);
    
    boost::unit_test::test_suite *    test =
        BOOST_TEST_SUITE("quaternion_test");
    
    BOOST_TEST_MESSAGE("Results of quaternion test.");
    BOOST_TEST_MESSAGE(" ");
    BOOST_TEST_MESSAGE("(C) Copyright Hubert Holin 2003-2005.");
    BOOST_TEST_MESSAGE("Distributed under the Boost Software License, Version 1.0.");
    BOOST_TEST_MESSAGE("(See accompanying file LICENSE_1_0.txt or copy at");
    BOOST_TEST_MESSAGE("http://www.boost.org/LICENSE_1_0.txt)");
    BOOST_TEST_MESSAGE(" ");
    
#define    BOOST_QUATERNION_COMMON_GENERATOR(fct)   \
    test->add(BOOST_TEST_CASE_TEMPLATE(fct##_test, test_types));

#define    BOOST_QUATERNION_COMMON_GENERATOR_NEAR_EPS(fct)   \
    test->add(BOOST_TEST_CASE_TEMPLATE(fct##_test, near_eps_test_types));
    
    
#define    BOOST_QUATERNION_TEST                      \
    BOOST_QUATERNION_COMMON_GENERATOR(multiplication) \
    BOOST_QUATERNION_COMMON_GENERATOR_NEAR_EPS(exp)            \
    BOOST_QUATERNION_COMMON_GENERATOR_NEAR_EPS(cos)            \
    BOOST_QUATERNION_COMMON_GENERATOR_NEAR_EPS(sin)            \
    BOOST_QUATERNION_COMMON_GENERATOR_NEAR_EPS(cosh)           \
    BOOST_QUATERNION_COMMON_GENERATOR_NEAR_EPS(sinh)
    
    
    BOOST_QUATERNION_TEST
    
    
#undef    BOOST_QUATERNION_TEST
    
#undef    BOOST_QUATERNION_COMMON_GENERATOR
#undef BOOST_QUATERNION_COMMON_GENERATOR_NEAR_EPS
    
#ifdef BOOST_QUATERNION_TEST_VERBOSE
    
    test->add(BOOST_TEST_CASE(quaternion_manual_test));
    
#endif    /* BOOST_QUATERNION_TEST_VERBOSE */
    
    return test;
}

#undef DEFINE_TYPE_NAME
