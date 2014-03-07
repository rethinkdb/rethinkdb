///////////////////////////////////////////////////////////////
//  Copyright Christopher Kormanyos 2002 - 2011.
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_
//
// This work is based on an earlier work:
// "Algorithm 910: A Portable C++ Multiple-Precision System for Special-Function Calculations",
// in ACM TOMS, {VOL 37, ISSUE 4, (February 2011)} (C) ACM, 2011. http://doi.acm.org/10.1145/1916461.1916469

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <boost/detail/lightweight_test.hpp>
#include <boost/array.hpp>
#include "test.hpp"

#if !defined(TEST_MPF_50) && !defined(TEST_MPF) && !defined(TEST_BACKEND) && !defined(TEST_CPP_DEC_FLOAT) && !defined(TEST_MPFR) && !defined(TEST_MPFR_50) && !defined(TEST_MPFI_50) && !defined(TEST_FLOAT128)
#  define TEST_MPF_50
//#  define TEST_MPF
#  define TEST_BACKEND
#  define TEST_CPP_DEC_FLOAT
#  define TEST_MPFI_50
#  define TEST_FLOAT128

#ifdef _MSC_VER
#pragma message("CAUTION!!: No backend type specified so testing everything.... this will take some time!!")
#endif
#ifdef __GNUC__
#pragma warning "CAUTION!!: No backend type specified so testing everything.... this will take some time!!"
#endif

#endif

#if defined(TEST_MPF_50)
#include <boost/multiprecision/gmp.hpp>
#endif
#if defined(TEST_MPFR_50)
#include <boost/multiprecision/mpfr.hpp>
#endif
#if defined(TEST_MPFI_50)
#include <boost/multiprecision/mpfi.hpp>
#endif
#ifdef TEST_BACKEND
#include <boost/multiprecision/concepts/mp_number_archetypes.hpp>
#endif
#ifdef TEST_CPP_DEC_FLOAT
#include <boost/multiprecision/cpp_dec_float.hpp>
#endif
#ifdef TEST_FLOAT128
#include <boost/multiprecision/float128.hpp>
#endif

template <class T>
void test()
{
   std::cout << "Testing type: " << typeid(T).name() << std::endl;
   //
   // Test with some exact binary values as input - this tests our code
   // rather than the test data:
   //
   static const boost::array<boost::array<T, 2>, 6> exact_data =
   {{
      {{ 0.5, static_cast<T>("0.523598775598298873077107230546583814032861566562517636829157432051302734381034833104672470890352844663691347752213717775") }},
      {{ 0.25, static_cast<T>("0.252680255142078653485657436993710972252193733096838193633923778740575060481021222411748742228014601605092602909414066566") }},
      {{0.75, static_cast<T>("0.848062078981481008052944338998418080073366213263112642860718163570200821228474234349189801731957230300995227265307531834") }},
      {{std::ldexp(1.0, -20), static_cast<T>("9.53674316406394560289664793089102218648031077292419572854816420395098616062014311172490017625353237219958438022056661501e-7") }},
      {{ 1 - std::ldexp(1.0, -20), static_cast<T>("1.56941525875313420204921285316218397515809899320201864334535204504240776023375739189119474528488143494473216475057072728") }},
      {{ 1, static_cast<T>("1.57079632679489661923132169163975144209858469968755291048747229615390820314310449931401741267105853399107404325664115332354692230477529111586267970406424055872514205135096926055277982231147447746519098221440548783296672306423782411689339158263560095457282428346173017430522716332410669680363012457064") }},
   }};
   unsigned max_err = 0;
   for(unsigned k = 0; k < exact_data.size(); k++)
   {
      T val = asin(exact_data[k][0]);
      T e = relative_error(val, exact_data[k][1]);
      unsigned err = e.template convert_to<unsigned>();
      if(err > max_err)
         max_err = err;
      val = asin(-exact_data[k][0]);
      e = relative_error(val, T(-exact_data[k][1]));
      err = e.template convert_to<unsigned>();
      if(err > max_err)
      {
         max_err = err;
      }
   }
   std::cout << "Max error was: " << max_err << std::endl;
   BOOST_TEST(max_err < 20);
   BOOST_TEST(asin(T(0)) == 0);
}


int main()
{
#ifdef TEST_BACKEND
   test<boost::multiprecision::number<boost::multiprecision::concepts::number_backend_float_architype> >();
#endif
#ifdef TEST_MPF_50
   test<boost::multiprecision::mpf_float_50>();
   test<boost::multiprecision::mpf_float_100>();
#endif
#ifdef TEST_MPFR_50
   test<boost::multiprecision::mpfr_float_50>();
   test<boost::multiprecision::mpfr_float_100>();
#endif
#ifdef TEST_MPFI_50
   test<boost::multiprecision::mpfi_float_50>();
   test<boost::multiprecision::mpfi_float_100>();
#endif
#ifdef TEST_CPP_DEC_FLOAT
   test<boost::multiprecision::cpp_dec_float_50>();
   test<boost::multiprecision::cpp_dec_float_100>();
#ifndef SLOW_COMPLER
   // Some "peculiar" digit counts which stress our code:
   test<boost::multiprecision::number<boost::multiprecision::cpp_dec_float<65> > >();
   test<boost::multiprecision::number<boost::multiprecision::cpp_dec_float<64> > >();
   test<boost::multiprecision::number<boost::multiprecision::cpp_dec_float<63> > >();
   test<boost::multiprecision::number<boost::multiprecision::cpp_dec_float<62> > >();
   test<boost::multiprecision::number<boost::multiprecision::cpp_dec_float<61, long long> > >();
   test<boost::multiprecision::number<boost::multiprecision::cpp_dec_float<60, long long> > >();
   test<boost::multiprecision::number<boost::multiprecision::cpp_dec_float<59, long long, std::allocator<void> > > >();
   test<boost::multiprecision::number<boost::multiprecision::cpp_dec_float<58, long long, std::allocator<void> > > >();
   // Check low multiprecision digit counts.
   test<boost::multiprecision::number<boost::multiprecision::cpp_dec_float<9> > >();
   test<boost::multiprecision::number<boost::multiprecision::cpp_dec_float<18> > >();
#endif
#endif
#ifdef TEST_FLOAT128
   test<boost::multiprecision::float128>();
#endif
   return boost::report_errors();
}



