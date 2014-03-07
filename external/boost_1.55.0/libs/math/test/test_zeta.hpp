// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2007, 2009
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/concepts/real_concept.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/tools/stats.hpp>
#include <boost/math/tools/test.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/array.hpp>
#include "functor.hpp"

#include "handle_test_result.hpp"
#include "test_zeta_hooks.hpp"
#include "table_type.hpp"

#ifndef SC_
#define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class Real, class T>
void do_test_zeta(const T& data, const char* type_name, const char* test_name)
{
   //
   // test zeta(T) against data:
   //
   using namespace std;
   typedef Real                   value_type;

   std::cout << test_name << " with type " << type_name << std::endl;

   typedef value_type (*pg)(value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::zeta<value_type>;
#else
   pg funcp = boost::math::zeta;
#endif

   boost::math::tools::test_result<value_type> result;
   //
   // test zeta against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0),
      extract_result<Real>(1));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::zeta", test_name);
#ifdef TEST_OTHER
   if(boost::is_floating_point<value_type>::value)
   {
      funcp = other::zeta;

      result = boost::math::tools::test_hetero<Real>(
         data,
         bind_func<Real>(funcp, 0),
         extract_result<Real>(1));
      handle_test_result(result, data[result.worst()], result.worst(), type_name, "other::zeta", test_name);
   }
#endif
   std::cout << std::endl;
}
template <class T>
void test_zeta(T, const char* name)
{
   //
   // The actual test data is rather verbose, so it's in a separate file
   //
#include "zeta_data.ipp"
   do_test_zeta<T>(zeta_data, name, "Zeta: Random values greater than 1");
#include "zeta_neg_data.ipp"
   do_test_zeta<T>(zeta_neg_data, name, "Zeta: Random values less than 1");
#include "zeta_1_up_data.ipp"
   do_test_zeta<T>(zeta_1_up_data, name, "Zeta: Values close to and greater than 1");
#include "zeta_1_below_data.ipp"
   do_test_zeta<T>(zeta_1_below_data, name, "Zeta: Values close to and less than 1");
}

extern "C" double zetac(double);

template <class T>
void test_spots(T, const char* t)
{
   std::cout << "Testing basic sanity checks for type " << t << std::endl;
   //
   // Basic sanity checks, tolerance is either 5 or 10 epsilon 
   // expressed as a percentage:
   //
   BOOST_MATH_STD_USING
   T tolerance = boost::math::tools::epsilon<T>() * 100 *
      (boost::is_floating_point<T>::value ? 5 : 10);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(0.125)), static_cast<T>(-0.63277562349869525529352526763564627152686379131122L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(1023) / static_cast<T>(1024)), static_cast<T>(-1023.4228554489429786541032870895167448906103303056L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(1025) / static_cast<T>(1024)), static_cast<T>(1024.5772867695045940578681624248887776501597556226L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(0.5)), static_cast<T>(-1.46035450880958681288949915251529801246722933101258149054289L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(1.125)), static_cast<T>(8.5862412945105752999607544082693023591996301183069L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(2)), static_cast<T>(1.6449340668482264364724151666460251892189499012068L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(3.5)), static_cast<T>(1.1267338673170566464278124918549842722219969574036L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(4)), static_cast<T>(1.08232323371113819151600369654116790277475095191872690768298L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(4 + static_cast<T>(1) / 1024), static_cast<T>(1.08225596856391369799036835439238249195298434901488518878804L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(4.5)), static_cast<T>(1.05470751076145426402296728896028011727249383295625173068468L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(6.5)), static_cast<T>(1.01200589988852479610078491680478352908773213619144808841031L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(7.5)), static_cast<T>(1.00582672753652280770224164440459408011782510096320822989663L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(8.125)), static_cast<T>(1.0037305205308161603183307711439385250181080293472L), 2 * tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(16.125)), static_cast<T>(1.0000140128224754088474783648500235958510030511915L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(0)), static_cast<T>(-0.5L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(-0.125)), static_cast<T>(-0.39906966894504503550986928301421235400280637468895L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(-1)), static_cast<T>(-0.083333333333333333333333333333333333333333333333333L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(-2)), static_cast<T>(0L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(-2.5)), static_cast<T>(0.0085169287778503305423585670283444869362759902200745L), tolerance * 3);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(-3)), static_cast<T>(0.0083333333333333333333333333333333333333333333333333L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(-4)), static_cast<T>(0), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(-20)), static_cast<T>(0), tolerance * 100);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(-21)), static_cast<T>(-281.46014492753623188405797101449275362318840579710L), tolerance * 100);
   BOOST_CHECK_CLOSE(::boost::math::zeta(static_cast<T>(-30.125)), static_cast<T>(2.2762941726834511267740045451463455513839970804578e7L), tolerance * 100);
   // Very small values:
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -20)), static_cast<T>(-0.500000876368989859479646132126454890645615288202492097957612L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -21)), static_cast<T>(-0.500000438184266833093492063649184012943132422189989164545507L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -22)), static_cast<T>(-0.500000219092076392425852854644256723571669269957526445270374L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -23)), static_cast<T>(-0.500000109546023940187789325464529558825433290921168958481804L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -24)), static_cast<T>(-0.500000054773008406088246161057525197302821575823476487961574L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -25)), static_cast<T>(-0.500000027386503312042790426817221131071450407798601059264341L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -26)), static_cast<T>(-0.500000013693251433271071983943082871935521396740331377486886L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -27)), static_cast<T>(-0.500000006846625660947956426350389518286874288247134329498289L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -28)), static_cast<T>(-0.500000003423312816552083476988056486473169377162409806781384L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -29)), static_cast<T>(-0.500000001711656404795568073849512135664960180586820144333542L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -30)), static_cast<T>(-0.500000000855828201527665623188910582717329375986726355164261L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -31)), static_cast<T>(-0.500000000427914100546303208463654361814800355929815322493143L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -32)), static_cast<T>(-0.500000000213957050218769203487022003676593508474107873788445L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -33)), static_cast<T>(-0.500000000106978525095789001562046589421133388262409441738089L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(ldexp(static_cast<T>(1), -34)), static_cast<T>(-0.500000000053489262544495600736249301842352101231724731340202L), tolerance);

   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -20)), static_cast<T>(-0.499999123632834911086872289657767335473025908373776645822722L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -21)), static_cast<T>(-0.499999561816189359548137231641582253243376087534976981434190L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -22)), static_cast<T>(-0.499999780908037655734554449793729262345041281451929584703788L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -23)), static_cast<T>(-0.499999890454004571852312499433422838864632848598847415933664L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -24)), static_cast<T>(-0.499999945226998721921779295091241395945379526155584220813497L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -25)), static_cast<T>(-0.499999972613498469959715937215237923104705216198368099221577L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -26)), static_cast<T>(-0.499999986306749012229554607064736104475024094525587925697276L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -27)), static_cast<T>(-0.499999993153374450427200221401546739119918746163907954406855L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -28)), static_cast<T>(-0.499999996576687211291705684949926422460038672790251466963619L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -29)), static_cast<T>(-0.499999998288343602165379216634983519354686193860717726606017L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -30)), static_cast<T>(-0.499999999144171800212571199432213326524228740247618955829902L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -31)), static_cast<T>(-0.499999999572085899888755997191626615213504580792674808876724L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -32)), static_cast<T>(-0.499999999786042949889995597926798240562852438685508646794693L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -33)), static_cast<T>(-0.499999999893021474931402198791408471637626205588681812641711L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::zeta(-ldexp(static_cast<T>(1), -34)), static_cast<T>(-0.499999999946510737462302199352114463422268928922372277519378L), tolerance);
}

