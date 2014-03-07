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
T cyl_bessel_i_int_wrapper(T v, T x)
{
   return static_cast<T>(
      boost::math::cyl_bessel_i(
      boost::math::itrunc(v), x));
}

template <class Real, class T>
void do_test_cyl_bessel_i(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::cyl_bessel_i<value_type, value_type>;
#else
   pg funcp = boost::math::cyl_bessel_i;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test cyl_bessel_i against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func<Real>(funcp, 0, 1), 
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::cyl_bessel_i", test_name);
   std::cout << std::endl;

#ifdef TEST_OTHER
   if(boost::is_floating_point<value_type>::value)
   {
      funcp = other::cyl_bessel_i;

      //
      // test other::cyl_bessel_i against data:
      //
      result = boost::math::tools::test(
         data, 
         boost::lambda::bind(funcp, 
            boost::lambda::ret<value_type>(boost::lambda::_1[0]),
            boost::lambda::ret<value_type>(boost::lambda::_1[1])), 
         boost::lambda::ret<value_type>(boost::lambda::_1[2]));
      print_test_result(result, data[result.worst()], result.worst(), type_name, "other::cyl_bessel_i");
      std::cout << std::endl;
   }
#endif
}

template <class Real, class T>
void do_test_cyl_bessel_i_int(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = cyl_bessel_i_int_wrapper<value_type>;
#else
   pg funcp = cyl_bessel_i_int_wrapper;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test cyl_bessel_i against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func<Real>(funcp, 0, 1), 
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::cyl_bessel_i", test_name);
   std::cout << std::endl;
}

template <class T>
void test_bessel(T, const char* name)
{
    // function values calculated on http://functions.wolfram.com/
    static const boost::array<boost::array<T, 3>, 10> i0_data = {{
        {{ SC_(0), SC_(0), SC_(1) }},
        {{ SC_(0), SC_(1), SC_(1.26606587775200833559824462521471753760767031135496220680814) }},
        {{ SC_(0), SC_(-2), SC_(2.27958530233606726743720444081153335328584110278545905407084) }},
        {{ SC_(0), SC_(4), SC_(11.3019219521363304963562701832171024974126165944353377060065) }},
        {{ SC_(0), SC_(-7), SC_(168.593908510289698857326627187500840376522679234531714193194) }},
        {{ SC_(0), T(1) / 1024, SC_(1.00000023841859331241759166109699567801556273303717896447683) }},
        {{ SC_(0), T(SC_(1)) / (1024*1024), SC_(1.00000000000022737367544324498417583090700894607432256476338) }},
        {{ SC_(0), SC_(-1), SC_(1.26606587775200833559824462521471753760767031135496220680814) }},
        {{ SC_(0), SC_(100), SC_(1.07375170713107382351972085760349466128840319332527279540154e42) }},
        {{ SC_(0), SC_(200), SC_(2.03968717340972461954167312677945962233267573614834337894328e85) }},
    }};
    static const boost::array<boost::array<T, 3>, 10> i1_data = {{
        {{ SC_(1), SC_(0), SC_(0) }},
        {{ SC_(1), SC_(1), SC_(0.565159103992485027207696027609863307328899621621092009480294) }},
        {{ SC_(1), SC_(-2), SC_(-1.59063685463732906338225442499966624795447815949553664713229) }},
        {{ SC_(1), SC_(4), SC_(9.75946515370444990947519256731268090005597033325296730692753) }},
        {{ SC_(1), SC_(-8), SC_(-399.873136782560098219083086145822754889628443904067647306574) }},
        {{ SC_(1), T(SC_(1))/1024, SC_(0.000488281308207663226432087816784315537514225208473395063575150) }},
        {{ SC_(1), T(SC_(1))/(1024*1024), SC_(4.76837158203179210108624277276025646653133998635956784292029E-7) }},
        {{ SC_(1), SC_(-10), SC_(-2670.98830370125465434103196677215254914574515378753771310849) }},
        {{ SC_(1), SC_(100), SC_(1.06836939033816248120614576322429526544612284405623226965918e42) }},
        {{ SC_(1), SC_(200), SC_(2.03458154933206270342742797713906950389661161681122964159220e85) }},
    }};
    static const boost::array<boost::array<T, 3>, 11> in_data = {{
        {{ SC_(-2), SC_(0), SC_(0) }},
        {{ SC_(2), T(SC_(1))/(1024*1024), SC_(1.13686837721624646204093977095674566928522671779753217215467e-13) }},
        {{ SC_(5), SC_(10), SC_(777.188286403259959907293484802339632852674154572666041953297) }},
        {{ SC_(-5), SC_(100), SC_(9.47009387303558124618275555002161742321578485033007130107740e41) }},
        {{ SC_(-5), SC_(-1), SC_(-0.000271463155956971875181073905153777342383564426758143634974124) }},
        {{ SC_(10), SC_(20), SC_(3.54020020901952109905289138244985607057267103782948493874391e6) }},
        {{ SC_(10), SC_(-5), SC_(0.00458004441917605126118647027872016953192323139337073320016447) }},
        {{ SC_(1e+02), SC_(9), SC_(2.74306601746058997093587654668959071522869282506446891736820e-93) }},
        {{ SC_(1e+02), SC_(80), SC_(4.65194832850610205318128191404145885093970505338730540776711e8) }},
        {{ SC_(-100), SC_(-200), SC_(4.35275044972702191438729017441198257508190719030765213981307e74) }},
        {{ SC_(10), SC_(1e-100), SC_(2.69114445546737213403880070546737213403880070546737213403880e-1010) }},
    }};
    static const boost::array<boost::array<T, 3>, 10> iv_data = {{
        {{ SC_(2.25), T(1)/(1024*1024), SC_(2.34379212133481347189068464680335815256364262507955635911656e-15) }},
        {{ SC_(5.5), SC_(3.125), SC_(0.0583514045989371500460946536220735787163510569634133670181210) }},
        {{ T(-5) + T(1)/1024, SC_(2.125), SC_(0.0267920938009571023702933210070984416052633027166975342895062) }},
        {{ SC_(-5.5), SC_(10), SC_(597.577606961369169607937419869926705730305175364662688426534) }},
        {{ SC_(-5.5), SC_(100), SC_(9.22362906144706871737354069133813819358704200689067071415379e41) }},
        {{ T(-10486074)/(1024*1024), T(1)/1024, SC_(1.41474005665181350367684623930576333542989766867888186478185e35) }},
        {{ T(-10486074)/(1024*1024), SC_(50), SC_(1.07153277202900671531087024688681954238311679648319534644743e20) }},
        {{ T(144794)/1024, SC_(100), SC_(2066.27694757392660413922181531984160871678224178890247540320) }},
        {{ T(144794)/1024, SC_(200), SC_(2.23699739472246928794922868978337381373643889659337595319774e64) }},
        {{ T(-144794)/1024, SC_(100), SC_(2066.27694672763190927440969155740243346136463461655104698748) }},
    }};
    static const boost::array<boost::array<T, 3>, 5> iv_large_data = {{
        // Bug report https://svn.boost.org/trac/boost/ticket/5560:
        {{ SC_(-1), static_cast<T>(ldexp(0.5, -512)), SC_(1.86458518280005168582274132886573345934411788365010172356788e-155) }},
        {{ SC_(1),  static_cast<T>(ldexp(0.5, -512)), SC_(1.86458518280005168582274132886573345934411788365010172356788e-155) }},
        {{ SC_(-1.125), static_cast<T>(ldexp(0.5, -512)), SC_(-1.34963720853101363690381585556234820027343435206156667634081e173) }},
        {{ SC_(1.125),  static_cast<T>(ldexp(0.5, -512)), SC_(8.02269390325932403421158766283366891170783955777638875887348e-175) }},
        {{ SC_(0.5), static_cast<T>(ldexp(0.5, -683)), SC_(8.90597649117647254543282704099383321071493400182381039079219e-104) }},
    }};

    do_test_cyl_bessel_i<T>(i0_data, name, "Bessel I0: Mathworld Data");
    do_test_cyl_bessel_i<T>(i1_data, name, "Bessel I1: Mathworld Data");
    do_test_cyl_bessel_i<T>(in_data, name, "Bessel In: Mathworld Data");

    do_test_cyl_bessel_i_int<T>(i0_data, name, "Bessel I0: Mathworld Data (Integer Version)");
    do_test_cyl_bessel_i_int<T>(i1_data, name, "Bessel I1: Mathworld Data (Integer Version)");
    do_test_cyl_bessel_i_int<T>(in_data, name, "Bessel In: Mathworld Data (Integer Version)");

    do_test_cyl_bessel_i<T>(iv_data, name, "Bessel Iv: Mathworld Data");

#include "bessel_i_int_data.ipp"
    do_test_cyl_bessel_i<T>(bessel_i_int_data, name, "Bessel In: Random Data");
#include "bessel_i_data.ipp"
    do_test_cyl_bessel_i<T>(bessel_i_data, name, "Bessel Iv: Random Data");

    if(0 != static_cast<T>(ldexp(0.5, -700)))
      do_test_cyl_bessel_i<T>(iv_large_data, name, "Bessel Iv: Mathworld Data (large values)");
}

