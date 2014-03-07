// Copyright John Maddock 2006.
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/bindings/rr.hpp>
//#include <boost/math/tools/dual_precision.hpp>
#include <boost/math/tools/test_data.hpp>
#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/math/special_functions/ellint_3.hpp>
#include <fstream>
#include <boost/math/tools/test_data.hpp>
#include <boost/tr1/random.hpp>

float extern_val;
// confuse the compilers optimiser, and force a truncation to float precision:
float truncate_to_float(float const * pf)
{
   extern_val = *pf;
   return *pf;
}

boost::math::tuple<boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR> generate_rf_data(boost::math::ntl::RR n)
{
   static std::tr1::mt19937 r;
   std::tr1::uniform_real<float> ur(0, 1);
   std::tr1::uniform_int<int> ui(-100, 100);
   float x = ur(r);
   x = ldexp(x, ui(r));
   boost::math::ntl::RR xr(truncate_to_float(&x));
   float y = ur(r);
   y = ldexp(y, ui(r));
   boost::math::ntl::RR yr(truncate_to_float(&y));
   float z = ur(r);
   z = ldexp(z, ui(r));
   boost::math::ntl::RR zr(truncate_to_float(&z));

   boost::math::ntl::RR result = boost::math::ellint_rf(xr, yr, zr);
   return boost::math::make_tuple(xr, yr, zr, result);
}

boost::math::tuple<boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR> generate_rc_data(boost::math::ntl::RR n)
{
   static std::tr1::mt19937 r;
   std::tr1::uniform_real<float> ur(0, 1);
   std::tr1::uniform_int<int> ui(-100, 100);
   float x = ur(r);
   x = ldexp(x, ui(r));
   boost::math::ntl::RR xr(truncate_to_float(&x));
   float y = ur(r);
   y = ldexp(y, ui(r));
   boost::math::ntl::RR yr(truncate_to_float(&y));

   boost::math::ntl::RR result = boost::math::ellint_rc(xr, yr);
   return boost::math::make_tuple(xr, yr, result);
}

boost::math::tuple<boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR> generate_rj_data(boost::math::ntl::RR n)
{
   static std::tr1::mt19937 r;
   std::tr1::uniform_real<float> ur(0, 1);
   std::tr1::uniform_real<float> nur(-1, 1);
   std::tr1::uniform_int<int> ui(-100, 100);
   float x = ur(r);
   x = ldexp(x, ui(r));
   boost::math::ntl::RR xr(truncate_to_float(&x));
   float y = ur(r);
   y = ldexp(y, ui(r));
   boost::math::ntl::RR yr(truncate_to_float(&y));
   float z = ur(r);
   z = ldexp(z, ui(r));
   boost::math::ntl::RR zr(truncate_to_float(&z));
   float p = nur(r);
   p = ldexp(p, ui(r));
   boost::math::ntl::RR pr(truncate_to_float(&p));

   boost::math::ellint_rj(x, y, z, p);

   boost::math::ntl::RR result = boost::math::ellint_rj(xr, yr, zr, pr);
   return boost::math::make_tuple(xr, yr, zr, pr, result);
}

boost::math::tuple<boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR, boost::math::ntl::RR> generate_rd_data(boost::math::ntl::RR n)
{
   static std::tr1::mt19937 r;
   std::tr1::uniform_real<float> ur(0, 1);
   std::tr1::uniform_int<int> ui(-100, 100);
   float x = ur(r);
   x = ldexp(x, ui(r));
   boost::math::ntl::RR xr(truncate_to_float(&x));
   float y = ur(r);
   y = ldexp(y, ui(r));
   boost::math::ntl::RR yr(truncate_to_float(&y));
   float z = ur(r);
   z = ldexp(z, ui(r));
   boost::math::ntl::RR zr(truncate_to_float(&z));

   boost::math::ntl::RR result = boost::math::ellint_rd(xr, yr, zr);
   return boost::math::make_tuple(xr, yr, zr, result);
}

int cpp_main(int argc, char*argv [])
{
   using namespace boost::math::tools;

   boost::math::ntl::RR::SetOutputPrecision(50);
   boost::math::ntl::RR::SetPrecision(1000);

   parameter_info<boost::math::ntl::RR> arg1, arg2;
   test_data<boost::math::ntl::RR> data;

   bool cont;
   std::string line;

   if(argc < 1)
      return 1;

   do{
      int count;
      std::cout << "Number of points: ";
      std::cin >> count;
      
      arg1 = make_periodic_param(boost::math::ntl::RR(0), boost::math::ntl::RR(1), count);
      arg1.type |= dummy_param;

      //
      // Change this next line to get the R variant you want:
      //
      data.insert(&generate_rd_data, arg1);

      std::cout << "Any more data [y/n]?";
      std::getline(std::cin, line);
      boost::algorithm::trim(line);
      cont = (line == "y");
   }while(cont);

   std::cout << "Enter name of test data file [default=ellint_rf_data.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "ellint_rf_data.ipp";
   std::ofstream ofs(line.c_str());
   line.erase(line.find('.'));
   ofs << std::scientific;
   write_code(ofs, data, line.c_str());

   return 0;
}


