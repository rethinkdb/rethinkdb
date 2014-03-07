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
#include <boost/math/constants/constants.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/array.hpp>
#include "functor.hpp"

#include "handle_test_result.hpp"
#include "test_bessel_hooks.hpp"
#include "table_type.hpp"

#ifndef SC_
#  define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class Real, class T>
void do_test_cyl_neumann_y(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::cyl_neumann<value_type, value_type>;
#else
   pg funcp = boost::math::cyl_neumann;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test cyl_neumann against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func<Real>(funcp, 0, 1), 
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::cyl_neumann", test_name);
   std::cout << std::endl;

#ifdef TEST_OTHER
   if(boost::is_floating_point<value_type>::value)
   {
      funcp = other::cyl_neumann;

      //
      // test other::cyl_neumann against data:
      //
      result = boost::math::tools::test_hetero<Real>(
         data, 
         bind_func<Real>(funcp, 0, 1), 
         extract_result<Real>(2));
      handle_test_result(result, data[result.worst()], result.worst(), type_name, "other::cyl_neumann", test_name);
      std::cout << std::endl;
   }
#endif
}

template <class T>
T cyl_neumann_int_wrapper(T v, T x)
{
   return static_cast<T>(boost::math::cyl_neumann(boost::math::itrunc(v), x));
}

template <class Real, class T>
void do_test_cyl_neumann_y_int(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = cyl_neumann_int_wrapper<value_type>;
#else
   pg funcp = cyl_neumann_int_wrapper;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test cyl_neumann against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func<Real>(funcp, 0, 1), 
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::cyl_neumann", test_name);
   std::cout << std::endl;
}

template <class Real, class T>
void do_test_sph_neumann_y(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(unsigned, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::sph_neumann<value_type>;
#else
   pg funcp = boost::math::sph_neumann;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test sph_neumann against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func_int1<Real>(funcp, 0, 1), 
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::cyl_neumann", test_name);
   std::cout << std::endl;
}

template <class T>
void test_bessel(T, const char* name)
{
   //
   // The actual test data is rather verbose, so it's in a separate file
   //
   // The contents are as follows, each row of data contains
   // three items, input value a, input value b and erf(a, b):
   // 
    // function values calculated on http://functions.wolfram.com/
    static const boost::array<boost::array<typename table_type<T>::type, 3>, 9> y0_data = {{
        {{ SC_(0), SC_(1), SC_(0.0882569642156769579829267660235151628278175230906755467110438) }},
        {{ SC_(0), SC_(2), SC_(0.510375672649745119596606592727157873268139227085846135571839) }},
        {{ SC_(0), SC_(4), SC_(-0.0169407393250649919036351344471532182404925898980149027169321) }},
        {{ SC_(0), SC_(8), SC_(0.223521489387566220527323400498620359274814930781423577578334) }},
        {{ SC_(0), SC_(1e-05), SC_(-7.40316028370197013259676050746759072070960287586102867247159) }},
        {{ SC_(0), SC_(1e-10), SC_(-14.7325162726972420426916696426209144888762342592762415255386) }},
        {{ SC_(0), SC_(1e-20), SC_(-29.3912282502857968601858410375186700783698345615477536431464) }},
        {{ SC_(0), SC_(1e+03), SC_(0.00471591797762281339977326146566525500985900489680197718528000) }},
        {{ SC_(0), SC_(1e+05), SC_(0.00184676615886506410434074102431546125884886798090392516843524) }}
    }};
    static const boost::array<boost::array<typename table_type<T>::type, 3>, 9> y1_data = {{
        {{ SC_(1), SC_(1), SC_(-0.781212821300288716547150000047964820549906390716444607843833) }},
        {{ SC_(1), SC_(2), SC_(-0.107032431540937546888370772277476636687480898235053860525795) }},
        {{ SC_(1), SC_(4), SC_(0.397925710557100005253979972450791852271189181622908340876586) }},
        {{ SC_(1), SC_(8), SC_(-0.158060461731247494255555266187483550327344049526705737651263) }},
        {{ SC_(1), SC_(1e-10), SC_(-6.36619772367581343150789184284462611709080831190542841855708e9) }},
        {{ SC_(1), SC_(1e-20), SC_(-6.36619772367581343075535053490057448139324059868649274367256e19) }},
        {{ SC_(1), SC_(1e+01), SC_(0.249015424206953883923283474663222803260416543069658461246944) }},
        {{ SC_(1), SC_(1e+03), SC_(-0.0247843312923517789148623560971412909386318548648705287583490) }},
        {{ SC_(1), SC_(1e+05), SC_(0.00171921035008825630099494523539897102954509504993494957572726) }}
    }};
    static const boost::array<boost::array<typename table_type<T>::type, 3>, 10> yn_data = {{
        {{ SC_(2), SC_(1e-20), SC_(-1.27323954473516268615107010698011489627570899691226996904849e40) }},
        {{ SC_(5), SC_(10), SC_(0.135403047689362303197029014762241709088405766746419538495983) }},
        {{ SC_(-5), SC_(1e+06), SC_(0.000331052088322609048503535570014688967096938338061796192422114) }},
        {{ SC_(10), SC_(10), SC_(-0.359814152183402722051986577343560609358382147846904467526222) }},
        {{ SC_(10), SC_(1e-10), SC_(-1.18280490494334933900960937719565669877576135140014365217993e108) }},
        {{ SC_(-10), SC_(1e+06), SC_(0.000725951969295187086245251366365393653610914686201194434805730) }},
        {{ SC_(1e+02), SC_(5), SC_(-5.08486391602022287993091563093082035595081274976837280338134e115) }},
        {{ SC_(1e+03), SC_(1e+05), SC_(0.00217254919137684037092834146629212647764581965821326561261181) }},
        {{ SC_(-1e+03), SC_(7e+02), SC_(-1.88753109980945889960843803284345261796244752396992106755091e77) }},
        {{ SC_(-25), SC_(8), SC_(3.45113613777297661997458045843868931827873456761831907587263e8) }}
    }};
    static const boost::array<boost::array<T, 3>, 11> yv_data = {{
        //SC_(2.25), {{ SC_(1) / 1024, SC_(-1.01759203636941035147948317764932151601257765988969544340275e7) }},
        {{ SC_(0.5), T(1) / (1024*1024), SC_(-817.033790261762580469303126467917092806755460418223776544122) }},
        {{ SC_(5.5), SC_(3.125), SC_(-2.61489440328417468776474188539366752698192046890955453259866) }},
        {{ SC_(-5.5), SC_(3.125), SC_(-0.0274994493896489729948109971802244976377957234563871795364056) }},
        {{ SC_(-5.5), SC_(1e+04), SC_(-0.00759343502722670361395585198154817047185480147294665270646578) }},
        {{ T(-10486074) / (1024*1024), T(1)/1024, SC_(-1.50382374389531766117868938966858995093408410498915220070230e38) }},
        {{ T(-10486074) / (1024*1024), SC_(1e+02), SC_(0.0583041891319026009955779707640455341990844522293730214223545) }},
        {{ SC_(141.75), SC_(1e+02), SC_(-5.38829231428696507293191118661269920130838607482708483122068e9) }},
        {{ SC_(141.75), SC_(2e+04), SC_(-0.00376577888677186194728129112270988602876597726657372330194186) }},
        {{ SC_(-141.75), SC_(1e+02), SC_(-3.81009803444766877495905954105669819951653361036342457919021e9) }},
        {{ SC_(8.5), boost::math::constants::pi<T>() * 4, SC_(0.257086543428224355151772807588810984369026142375675714560864) }},
        {{ SC_(-8.5), boost::math::constants::pi<T>() * 4, SC_(0.0436807946352780974532519564114026730332781693877984686758680) }},
    }};
    static const boost::array<boost::array<T, 3>, 7> yv_large_data = {{
        // Bug report https://svn.boost.org/trac/boost/ticket/5560:
        {{ SC_(0.5), static_cast<T>(std::ldexp(0.5, -683)), SC_(-7.14823099969225685526188875418476476336424046896822867989728e102) }},
        {{ SC_(-0.5), static_cast<T>(std::ldexp(0.5, -683)), SC_(8.90597649117647254543282704099383321071493400182381039079219e-104) }},
        {{ SC_(0.0), static_cast<T>(std::ldexp(1.0, -53)), SC_(-23.4611779112897561252987257324561640034037313549011724328997) }},
        {{ SC_(1.0), static_cast<T>(std::ldexp(1.0, -53)), SC_(-5.73416113922265864550047623401604244038331542638719289100990e15) }},
        {{ SC_(2.0), static_cast<T>(std::ldexp(1.0, -53)), SC_(-1.03297463879542177245046832533417970379386617249046560049244e32) }},
        {{ SC_(3.0), static_cast<T>(std::ldexp(1.0, -53)), SC_(-3.72168335868978735639260528876490232745489151562358712422544e48) }},
        {{ SC_(10.0), static_cast<T>(std::ldexp(1.0, -53)), SC_(-4.15729476804920974669173904282420477878640623992500096231384e167) }},
    }};

    do_test_cyl_neumann_y<T>(y0_data, name, "Y0: Mathworld Data");
    do_test_cyl_neumann_y<T>(y1_data, name, "Y1: Mathworld Data");
    do_test_cyl_neumann_y<T>(yn_data, name, "Yn: Mathworld Data");
    do_test_cyl_neumann_y_int<T>(y0_data, name, "Y0: Mathworld Data (Integer Version)");
    do_test_cyl_neumann_y_int<T>(y1_data, name, "Y1: Mathworld Data (Integer Version)");
    do_test_cyl_neumann_y_int<T>(yn_data, name, "Yn: Mathworld Data (Integer Version)");
    do_test_cyl_neumann_y<T>(yv_data, name, "Yv: Mathworld Data");
    if(yv_large_data[0][1] != 0)
      do_test_cyl_neumann_y<T>(yv_large_data, name, "Yv: Mathworld Data (large values)");

#include "bessel_y01_data.ipp"
    do_test_cyl_neumann_y<T>(bessel_y01_data, name, "Y0 and Y1: Random Data");
#include "bessel_yn_data.ipp"
    do_test_cyl_neumann_y<T>(bessel_yn_data, name, "Yn: Random Data");
#include "bessel_yv_data.ipp"
    do_test_cyl_neumann_y<T>(bessel_yv_data, name, "Yv: Random Data");

#include "sph_neumann_data.ipp"
    do_test_sph_neumann_y<T>(sph_neumann_data, name, "y: Random Data");
}

