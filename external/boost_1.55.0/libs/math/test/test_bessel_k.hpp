//  (C) Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_MATH_OVERFLOW_ERROR_POLICY ignore_error
#include <boost/math/concepts/real_concept.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/array.hpp>
#include "functor.hpp"

#include "handle_test_result.hpp"
#include "test_bessel_hooks.hpp"
#include "table_type.hpp"

#ifndef SC_
#  define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class T>
T cyl_bessel_k_int_wrapper(T v, T x)
{
   return static_cast<T>(
      boost::math::cyl_bessel_k(
      boost::math::itrunc(v), x));
}

template <class Real, class T>
void do_test_cyl_bessel_k(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::cyl_bessel_k<value_type, value_type>;
#else
   pg funcp = boost::math::cyl_bessel_k;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test cyl_bessel_k against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func<Real>(funcp, 0, 1), 
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::cyl_bessel_k", test_name);
   std::cout << std::endl;

#ifdef TEST_OTHER
   if(boost::is_floating_point<value_type>::value)
   {
      funcp = other::cyl_bessel_k;

      //
      // test other::cyl_bessel_k against data:
      //
      result = boost::math::tools::test_hetero(
         data, 
         bind_func<Real>(funcp, 0, 1), 
         extract_result<Real>(2));
      print_test_result(result, data[result.worst()], result.worst(), type_name, "other::cyl_bessel_k");
      std::cout << std::endl;
   }
#endif
}

template <class Real, class T>
void do_test_cyl_bessel_k_int(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = cyl_bessel_k_int_wrapper<value_type>;
#else
   pg funcp = cyl_bessel_k_int_wrapper;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test cyl_bessel_k against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func<Real>(funcp, 0, 1), 
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::cyl_bessel_k", test_name);
   std::cout << std::endl;
}

template <class T>
void test_bessel(T, const char* name)
{
    // function values calculated on http://functions.wolfram.com/
    static const boost::array<boost::array<T, 3>, 9> k0_data = {{
        {{ SC_(0), SC_(1), SC_(0.421024438240708333335627379212609036136219748226660472298970) }},
        {{ SC_(0), SC_(2), SC_(0.113893872749533435652719574932481832998326624388808882892530) }},
        {{ SC_(0), SC_(4), SC_(0.0111596760858530242697451959798334892250090238884743405382553) }},
        {{ SC_(0), SC_(8), SC_(0.000146470705222815387096584408698677921967305368833759024089154) }},
        {{ SC_(0), T(std::ldexp(1.0, -15)), SC_(10.5131392267382037062459525561594822400447325776672021972753) }},
        {{ SC_(0), T(std::ldexp(1.0, -30)), SC_(20.9103469324567717360787328239372191382743831365906131108531) }},
        {{ SC_(0), T(std::ldexp(1.0, -60)), SC_(41.7047623492551310138446473188663682295952219631968830346918) }},
        {{ SC_(0), SC_(50), SC_(3.41016774978949551392067551235295223184502537762334808993276e-23) }},
        {{ SC_(0), SC_(100), SC_(4.65662822917590201893900528948388635580753948544211387402671e-45) }},
    }};
    static const boost::array<boost::array<T, 3>, 9> k1_data = {{
        {{ SC_(1), SC_(1), SC_(0.601907230197234574737540001535617339261586889968106456017768) }},
        {{ SC_(1), SC_(2), SC_(0.139865881816522427284598807035411023887234584841515530384442) }},
        {{ SC_(1), SC_(4), SC_(0.0124834988872684314703841799808060684838415849886258457917076) }},
        {{ SC_(1), SC_(8), SC_(0.000155369211805001133916862450622474621117065122872616157079566) }},
        {{ SC_(1), T(std::ldexp(1.0, -15)), SC_(32767.9998319528316432647441316539139725104728341577594326513) }},
        {{ SC_(1), T(std::ldexp(1.0, -30)), SC_(1.07374182399999999003003028572687332810353799544215073362305e9) }},
        {{ SC_(1), T(std::ldexp(1.0, -60)), SC_(1.15292150460684697599999999999999998169660198868126604634036e18) }},
        {{ SC_(1), SC_(50), SC_(3.44410222671755561259185303591267155099677251348256880221927e-23) }},
        {{ SC_(1), SC_(100), SC_(4.67985373563690928656254424202433530797494354694335352937465e-45) }},
    }};
    static const boost::array<boost::array<T, 3>, 9> kn_data = {{
        {{ SC_(2), T(std::ldexp(1.0, -30)), SC_(2.30584300921369395150000000000000000234841952009593636868109e18) }},
        {{ SC_(5), SC_(10), SC_(0.0000575418499853122792763740236992723196597629124356739596921536) }},
        {{ SC_(-5), SC_(100), SC_(5.27325611329294989461777188449044716451716555009882448801072e-45) }},
        {{ SC_(10), SC_(10), SC_(0.00161425530039067002345725193091329085443750382929208307802221) }},
        {{ SC_(10), T(std::ldexp(1.0, -30)), SC_(3.78470202927236255215249281534478864916684072926050665209083e98) }},
        {{ SC_(-10), SC_(1), SC_(1.80713289901029454691597861302340015908245782948536080022119e8) }},
        {{ SC_(100), SC_(5), SC_(7.03986019306167654653386616796116726248616158936088056952477e115) }},
        {{ SC_(100), SC_(80), SC_(8.39287107246490782848985384895907681748152272748337807033319e-12) }},
        {{ SC_(-1000), SC_(700), SC_(6.51561979144735818903553852606383312984409361984128221539405e-31) }},
    }};
    static const boost::array<boost::array<T, 3>, 11> kv_data = {{
        {{ SC_(0.5), SC_(0.875), SC_(0.558532231646608646115729767013630967055657943463362504577189) }},
        {{ SC_(0.5), SC_(1.125), SC_(0.383621010650189547146769320487006220295290256657827220786527) }},
        {{ SC_(2.25), T(std::ldexp(1.0, -30)), SC_(5.62397392719283271332307799146649700147907612095185712015604e20) }},
        {{ SC_(5.5), T(3217)/1024, SC_(1.30623288775012596319554857587765179889689223531159532808379) }},
        {{ SC_(-5.5), SC_(10), SC_(0.0000733045300798502164644836879577484533096239574909573072142667) }},
        {{ SC_(-5.5), SC_(100), SC_(5.41274555306792267322084448693957747924412508020839543293369e-45) }},
        {{ T(10240)/1024, T(1)/1024, SC_(2.35522579263922076203415803966825431039900000000993410734978e38) }},
        {{ T(10240)/1024, SC_(10), SC_(0.00161425530039067002345725193091329085443750382929208307802221) }},
        {{ T(144793)/1024, SC_(100), SC_(1.39565245860302528069481472855619216759142225046370312329416e-6) }},
        {{ T(144793)/1024, SC_(200), SC_(9.11950412043225432171915100042647230802198254567007382956336e-68) }},
        {{ T(-144793)/1024, SC_(50), SC_(1.30185229717525025165362673848737761549946548375142378172956e42) }},
    }};
    static const boost::array<boost::array<T, 3>, 5> kv_large_data = {{
        // Bug report https://svn.boost.org/trac/boost/ticket/5560:
        {{ SC_(-1), static_cast<T>(ldexp(0.5, -512)), SC_(2.68156158598851941991480499964116922549587316411847867554471e154) }},
        {{ SC_(1),  static_cast<T>(ldexp(0.5, -512)), SC_(2.68156158598851941991480499964116922549587316411847867554471e154) }},
        {{ SC_(-1.125), static_cast<T>(ldexp(0.5, -512)), SC_(5.53984048006472105611199242328122729730752165907526178753978e173) }},
        {{ SC_(1.125),  static_cast<T>(ldexp(0.5, -512)), SC_(5.53984048006472105611199242328122729730752165907526178753978e173) }},
        {{ SC_(0.5),  static_cast<T>(ldexp(0.5, -683)), SC_(1.12284149973980088540335945247019177715948513804063794284101e103) }},
    }};

    do_test_cyl_bessel_k<T>(k0_data, name, "Bessel K0: Mathworld Data");
    do_test_cyl_bessel_k<T>(k1_data, name, "Bessel K1: Mathworld Data");
    do_test_cyl_bessel_k<T>(kn_data, name, "Bessel Kn: Mathworld Data");

    do_test_cyl_bessel_k_int<T>(k0_data, name, "Bessel K0: Mathworld Data (Integer Version)");
    do_test_cyl_bessel_k_int<T>(k1_data, name, "Bessel K1: Mathworld Data (Integer Version)");
    do_test_cyl_bessel_k_int<T>(kn_data, name, "Bessel Kn: Mathworld Data (Integer Version)");

    do_test_cyl_bessel_k<T>(kv_data, name, "Bessel Kv: Mathworld Data");
    if(0 != static_cast<T>(ldexp(0.5, -512)))
      do_test_cyl_bessel_k<T>(kv_large_data, name, "Bessel Kv: Mathworld Data (large values)");
#include "bessel_k_int_data.ipp"
    do_test_cyl_bessel_k<T>(bessel_k_int_data, name, "Bessel Kn: Random Data");
#include "bessel_k_data.ipp"
    do_test_cyl_bessel_k<T>(bessel_k_data, name, "Bessel Kv: Random Data");
}

