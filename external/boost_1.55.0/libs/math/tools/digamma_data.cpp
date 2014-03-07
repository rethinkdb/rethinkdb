//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/bindings/rr.hpp>
#include <boost/math/special_functions/digamma.hpp>
#include <boost/test/included/prg_exec_monitor.hpp>
#include <fstream>

#include <boost/math/tools/test_data.hpp>

using namespace boost::math::tools;
using namespace std;

float external_f;
float force_truncate(const float* f)
{
   external_f = *f;
   return external_f;
}

float truncate_to_float(boost::math::ntl::RR r)
{
   float f = boost::math::tools::real_cast<float>(r);
   return force_truncate(&f);
}

int cpp_main(int argc, char*argv [])
{
   boost::math::ntl::RR::SetPrecision(1000);
   boost::math::ntl::RR::SetOutputPrecision(40);

   parameter_info<boost::math::ntl::RR> arg1;
   test_data<boost::math::ntl::RR> data;

   bool cont;
   std::string line;

   std::cout << "Welcome.\n"
      "This program will generate spot tests for the digamma function:\n"
      "  digamma(z)\n\n";

   do{
      if(0 == get_user_parameter_info(arg1, "z"))
         return 1;
      data.insert(&boost::math::digamma<boost::math::ntl::RR>, arg1);

      std::cout << "Any more data [y/n]?";
      std::getline(std::cin, line);
      boost::algorithm::trim(line);
      cont = (line == "y");
   }while(cont);

   std::cout << "Enter name of test data file [default=digamma_data.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "digamma_data.ipp";
   std::ofstream ofs(line.c_str());
   write_code(ofs, data, "digamma_data");
   
   return 0;
}



