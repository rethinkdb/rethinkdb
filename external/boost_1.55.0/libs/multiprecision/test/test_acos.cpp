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
   static const boost::array<boost::array<T, 2>, 13> exact_data =
   {{
      {{ 0.5, static_cast<T>("1.04719755119659774615421446109316762806572313312503527365831486410260546876206966620934494178070568932738269550442743555") }},
      {{ 0.25, static_cast<T>("1.31811607165281796574566425464604046984639096659071471685354851741333314266208327690226867044304393238598144034722708676") }},
      {{0.75, static_cast<T>("0.722734247813415611178377352641333362025218486424440267626754132583707381914630264964827610939101303690078815991333621490") }},
      {{1 - std::ldexp(1.0, -20), static_cast<T>("0.00138106804176241718210883847756746694048570648553426714212025111150044290934710742282266738617709904634187850607042604204") }},
      {{std::ldexp(1.0, -20), static_cast<T>("1.57079537312058021283676140197495835299636605165647561806789944133748780804448843729970624018104090863783682329820313127") }},
      {{1, static_cast<T>("0") }},

      {{0, static_cast<T>("1.57079632679489661923132169163975144209858469968755291048747229615390820314310449931401741267105853399107404325664115332") }},

      {{ -0.5, static_cast<T>("2.09439510239319549230842892218633525613144626625007054731662972820521093752413933241868988356141137865476539100885487110") }},
      {{ -0.25, static_cast<T>("1.82347658193697527271697912863346241435077843278439110412139607489448326362412572172576615489907313559616664616605521989") }},
      {{-0.75, static_cast<T>("2.41885840577637762728426603063816952217195091295066555334819045972410902437157873366320721440301576429206927052194868516") }},
      {{-1 + std::ldexp(1.0, -20), static_cast<T>("3.14021158554803082128053454480193541725668369288957155383282434119631596337686189120521215795593996893580620800721188061") }},
      {{-std::ldexp(1.0, -20), static_cast<T>("1.57079728046921302562588198130454453120080334771863020290704515097032859824172056132832858516107615934431126321507917538") }},
      {{-1, static_cast<T>("3.14159265358979323846264338327950288419716939937510582097494459230781640628620899862803482534211706798214808651328230665") }},
   }};
   unsigned max_err = 0;
   for(unsigned k = 0; k < exact_data.size(); k++)
   {
      T val = acos(exact_data[k][0]);
      T e = relative_error(val, exact_data[k][1]);
      unsigned err = e.template convert_to<unsigned>();
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
#endif
#endif
#ifdef TEST_FLOAT128
   test<boost::multiprecision::float128>();
#endif
   return boost::report_errors();
}



