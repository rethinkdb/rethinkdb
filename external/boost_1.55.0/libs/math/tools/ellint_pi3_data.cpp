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

boost::math::tuple<boost::math::ntl::RR, boost::math::ntl::RR> generate_data(boost::math::ntl::RR n, boost::math::ntl::RR phi)
{
   static std::tr1::mt19937 r;
   std::tr1::uniform_real<float> ui(0, 1);
   float k = ui(r);
   boost::math::ntl::RR kr(truncate_to_float(&k));
   boost::math::ntl::RR result = boost::math::ellint_3(kr, n, phi);
   return boost::math::make_tuple(kr, result);
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
      if(0 == get_user_parameter_info(arg1, "n"))
         return 1;
      if(0 == get_user_parameter_info(arg2, "phi"))
         return 1;

      data.insert(&generate_data, arg1, arg2);

      std::cout << "Any more data [y/n]?";
      std::getline(std::cin, line);
      boost::algorithm::trim(line);
      cont = (line == "y");
   }while(cont);

   std::cout << "Enter name of test data file [default=ellint_pi3_data.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "ellint_pi3_data.ipp";
   std::ofstream ofs(line.c_str());
   line.erase(line.find('.'));
   ofs << std::scientific;
   write_code(ofs, data, line.c_str());

   return 0;
}


