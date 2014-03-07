// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2007, 2009
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/concepts/real_concept.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/tools/stats.hpp>
#include <boost/math/tools/test.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/array.hpp>
#include "functor.hpp"

#include "test_beta_hooks.hpp"
#include "handle_test_result.hpp"
#include "table_type.hpp"

#ifndef SC_
#define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class Real, class T>
void do_test_beta(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type, value_type, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::beta<value_type, value_type, value_type>;
#else
   pg funcp = boost::math::beta;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test beta against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0, 1, 2),
      extract_result<Real>(3));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::beta", test_name);

#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   funcp = boost::math::betac<value_type, value_type, value_type>;
#else
   funcp = boost::math::betac;
#endif
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0, 1, 2),
      extract_result<Real>(4));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::betac", test_name);

#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   funcp = boost::math::ibeta<value_type, value_type, value_type>;
#else
   funcp = boost::math::ibeta;
#endif
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0, 1, 2),
      extract_result<Real>(5));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::ibeta", test_name);

#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   funcp = boost::math::ibetac<value_type, value_type, value_type>;
#else
   funcp = boost::math::ibetac;
#endif
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0, 1, 2),
      extract_result<Real>(6));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::ibetac", test_name);
#ifdef TEST_OTHER
   if(::boost::is_floating_point<value_type>::value){
      funcp = other::ibeta;
      result = boost::math::tools::test_hetero<Real>(
         data,
         bind_func<Real>(funcp, 0, 1, 2),
         extract_result<Real>(5));
      print_test_result(result, data[result.worst()], result.worst(), type_name, "other::ibeta");
   }
#endif
   std::cout << std::endl;
}

template <class T>
void test_beta(T, const char* name)
{
   //
   // The actual test data is rather verbose, so it's in a separate file
   //
   // The contents are as follows, each row of data contains
   // five items, input value a, input value b, integration limits x, beta(a, b, x) and ibeta(a, b, x):
   //
#if !defined(TEST_DATA) || (TEST_DATA == 1)
#  include "ibeta_small_data.ipp"

   do_test_beta<T>(ibeta_small_data, name, "Incomplete Beta Function: Small Values");
#endif

#if !defined(TEST_DATA) || (TEST_DATA == 2)
#  include "ibeta_data.ipp"

   do_test_beta<T>(ibeta_data, name, "Incomplete Beta Function: Medium Values");

#endif
#if !defined(TEST_DATA) || (TEST_DATA == 3)
#  include "ibeta_large_data.ipp"

   do_test_beta<T>(ibeta_large_data, name, "Incomplete Beta Function: Large and Diverse Values");
#endif

#if !defined(TEST_DATA) || (TEST_DATA == 4)
#  include "ibeta_int_data.ipp"

   do_test_beta<T>(ibeta_int_data, name, "Incomplete Beta Function: Small Integer Values");
#endif
}

template <class T>
void test_spots(T)
{
   //
   // basic sanity checks, tolerance is 30 epsilon expressed as a percentage:
   // Spot values are from http://functions.wolfram.com/webMathematica/FunctionEvaluation.jsp?name=BetaRegularized
   // using precision of 50 decimal digits.
   T tolerance = boost::math::tools::epsilon<T>() * 3000;
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(159) / 10000, //(0.015964560210704803L),
         static_cast<T>(1184) / 1000000000L,//(1.1846856068586931e-005L),
         static_cast<T>(6917) / 10000),//(0.69176378846168518L)),
      static_cast<T>(0.000075393541456247525676062058821484095548666733251733L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(4243) / 100,//(42.434902191162109L),
         static_cast<T>(3001) / 10000, //(0.30012050271034241L),
         static_cast<T>(9157) / 10000), //(0.91574394702911377L)),
      static_cast<T>(0.0028387319012616013434124297160711532419664289474798L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(9713) / 1000, //(9.7131776809692383L),
         static_cast<T>(9940) / 100, //(99.406852722167969L),
         static_cast<T>(8391) / 100000), //(0.083912998437881470L)),
      static_cast<T>(0.46116895440368248909937863372410093344466819447476L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(72.5),
         static_cast<T>(1.125),
         static_cast<T>(0.75)),
      static_cast<T>(1.3423066982487051710597194786268004978931316494920e-9L), tolerance*3); // extra tolerance needed on linux X86EM64
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(4985)/1000, //(4.9854421615600586L),
         static_cast<T>(1066)/1000, //(1.0665277242660522L),
         static_cast<T>(7599)/10000), //(0.75997146964073181L)),
      static_cast<T>(0.27533431334486812211032939156910472371928659321347L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(6813)/1000, //(6.8127136230468750L),
         static_cast<T>(1056)/1000, //(1.0562920570373535L),
         static_cast<T>(1741)/10000), //(0.17416560649871826L)),
      static_cast<T>(7.6736128722762245852815040810349072461658078840945e-6L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(4898)/10000, //(0.48983201384544373L),
         static_cast<T>(2251)/10000, //(0.22512593865394592L),
         static_cast<T>(2003)/10000), //(0.20032680034637451L)),
      static_cast<T>(0.17089223868046209692215231702890838878342349377008L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(4049)/1000, //(4.0498137474060059L),
         static_cast<T>(1540)/10000, //(0.15403440594673157L),
         static_cast<T>(6537)/10000), //(0.65370121598243713L)),
      static_cast<T>(0.017273988301528087878279199511703371301647583919670L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(7269)/1000, //(7.2695474624633789L),
         static_cast<T>(1190)/10000, //(0.11902070045471191L),
         static_cast<T>(8003)/10000), //(0.80036874115467072L)),
      static_cast<T>(0.013334694467796052900138431733772122625376753696347L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(2726)/1000, //(2.7266697883605957L),
         static_cast<T>(1151)/100000, //(0.011510574258863926L),
         static_cast<T>(8665)/100000), //(0.086654007434844971L)),
      static_cast<T>(5.8218877068298586420691288375690562915515260230173e-6L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(3431)/10000, //(0.34317314624786377L),
         static_cast<T>(4634)/100000, //0.046342257410287857L),
         static_cast<T>(7582)/10000), //(0.75823287665843964L)),
      static_cast<T>(0.15132819929418661038699397753916091907278005695387L), tolerance);

   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(0.34317314624786377L),
         static_cast<T>(0.046342257410287857L),
         static_cast<T>(0)),
      static_cast<T>(0), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibetac(
         static_cast<T>(0.34317314624786377L),
         static_cast<T>(0.046342257410287857L),
         static_cast<T>(0)),
      static_cast<T>(1), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(0.34317314624786377L),
         static_cast<T>(0.046342257410287857L),
         static_cast<T>(1)),
      static_cast<T>(1), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibetac(
         static_cast<T>(0.34317314624786377L),
         static_cast<T>(0.046342257410287857L),
         static_cast<T>(1)),
      static_cast<T>(0), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(1),
         static_cast<T>(4634)/100000, //(0.046342257410287857L),
         static_cast<T>(32)/100),
      static_cast<T>(0.017712849440718489999419956301675684844663359595318L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(4634)/100000, //(0.046342257410287857L),
         static_cast<T>(1),
         static_cast<T>(32)/100),
      static_cast<T>(0.94856839398626914764591440181367780660208493234722L), tolerance);

   // try with some integer arguments:
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(3),
         static_cast<T>(8),
         static_cast<T>(0.25)),
      static_cast<T>(0.474407196044921875000000000000000000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(6),
         static_cast<T>(8),
         static_cast<T>(0.25)),
      static_cast<T>(0.0802125930786132812500000000000000000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(12),
         static_cast<T>(1),
         static_cast<T>(0.25)),
      static_cast<T>(5.96046447753906250000000000000000000000000000000000000000000e-8L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(1),
         static_cast<T>(8),
         static_cast<T>(0.25)),
      static_cast<T>(0.899887084960937500000000000000000000000000000000000000000000L), tolerance);

   // very naive check on derivative:
   using namespace std;  // For ADL of std functions
   tolerance = boost::math::tools::epsilon<T>() * 10000; // 100 eps
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta_derivative(
         static_cast<T>(2),
         static_cast<T>(3),
         static_cast<T>(0.5)),
         pow(static_cast<T>(0.5), static_cast<T>(2)) * pow(static_cast<T>(0.5), static_cast<T>(1)) / boost::math::beta(static_cast<T>(2), static_cast<T>(3)), tolerance);

   //
   // Special cases and error handling:
   //
   BOOST_CHECK_EQUAL(::boost::math::ibeta(static_cast<T>(0), static_cast<T>(2), static_cast<T>(0.5)), static_cast<T>(1));
   BOOST_CHECK_EQUAL(::boost::math::ibeta(static_cast<T>(3), static_cast<T>(0), static_cast<T>(0.5)), static_cast<T>(0));
   BOOST_CHECK_EQUAL(::boost::math::ibetac(static_cast<T>(0), static_cast<T>(2), static_cast<T>(0.5)), static_cast<T>(0));
   BOOST_CHECK_EQUAL(::boost::math::ibetac(static_cast<T>(4), static_cast<T>(0), static_cast<T>(0.5)), static_cast<T>(1));

   BOOST_CHECK_THROW(::boost::math::beta(static_cast<T>(0), static_cast<T>(2), static_cast<T>(0.5)), std::domain_error);
   BOOST_CHECK_THROW(::boost::math::beta(static_cast<T>(3), static_cast<T>(0), static_cast<T>(0.5)), std::domain_error);
   BOOST_CHECK_THROW(::boost::math::betac(static_cast<T>(0), static_cast<T>(2), static_cast<T>(0.5)), std::domain_error);
   BOOST_CHECK_THROW(::boost::math::betac(static_cast<T>(4), static_cast<T>(0), static_cast<T>(0.5)), std::domain_error);

   BOOST_CHECK_THROW(::boost::math::ibetac(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0.5)), std::domain_error);
   BOOST_CHECK_THROW(::boost::math::ibetac(static_cast<T>(-1), static_cast<T>(2), static_cast<T>(0.5)), std::domain_error);
   BOOST_CHECK_THROW(::boost::math::ibetac(static_cast<T>(2), static_cast<T>(-2), static_cast<T>(0.5)), std::domain_error);
   BOOST_CHECK_THROW(::boost::math::ibetac(static_cast<T>(2), static_cast<T>(2), static_cast<T>(-0.5)), std::domain_error);
   BOOST_CHECK_THROW(::boost::math::ibetac(static_cast<T>(2), static_cast<T>(2), static_cast<T>(1.5)), std::domain_error);

   //
   // a = b = 0.5 is a special case:
   //
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(0.5f),
         static_cast<T>(0.5f),
         static_cast<T>(0.25)),
      static_cast<T>(1) / 3, tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibetac(
         static_cast<T>(0.5f),
         static_cast<T>(0.5f),
         static_cast<T>(0.25)),
      static_cast<T>(2) / 3, tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(0.5f),
         static_cast<T>(0.5f),
         static_cast<T>(0.125)),
      static_cast<T>(0.230053456162615885213780567705142893009911395270714102055874L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibetac(
         static_cast<T>(0.5f),
         static_cast<T>(0.5f),
         static_cast<T>(0.125)),
      static_cast<T>(0.769946543837384114786219432294857106990088604729285897944125L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(0.5f),
         static_cast<T>(0.5f),
         static_cast<T>(0.825L)),
      static_cast<T>(0.725231121519469565327291851560156562956885802608457839260161L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibetac(
         static_cast<T>(0.5f),
         static_cast<T>(0.5f),
         static_cast<T>(0.825L)),
      static_cast<T>(0.274768878480530434672708148439843437043114197391542160739838L), tolerance);
   //
   // Second argument is 1 is a special case, see http://functions.wolfram.com/GammaBetaErf/BetaRegularized/03/01/01/
   //
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(0.5f),
         static_cast<T>(1),
         static_cast<T>(0.825L)),
      static_cast<T>(0.908295106229247499626759842915458109758420750043003849691665L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibetac(
         static_cast<T>(0.5f),
         static_cast<T>(1),
         static_cast<T>(0.825L)),
      static_cast<T>(0.091704893770752500373240157084541890241579249956996150308334L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibeta(
         static_cast<T>(30),
         static_cast<T>(1),
         static_cast<T>(0.825L)),
      static_cast<T>(0.003116150729395132012981654047222541793435357905008020740211L), tolerance);
   BOOST_CHECK_CLOSE(
      ::boost::math::ibetac(
         static_cast<T>(30),
         static_cast<T>(1),
         static_cast<T>(0.825L)),
      static_cast<T>(0.996883849270604867987018345952777458206564642094991979259788L), tolerance);
}

