// Copyright John Maddock 2006.
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/math/bindings/rr.hpp>
//#include <boost/math/tools/dual_precision.hpp>
#include <boost/math/tools/test_data.hpp>
#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/math/special_functions/ellint_2.hpp>
#include <boost/math/special_functions/ellint_3.hpp>
#include <fstream>
#include <boost/math/tools/test_data.hpp>
#include <boost/tr1/random.hpp>

using namespace boost::math::tools;
using namespace boost::math;
using namespace std;

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
      if(0 == get_user_parameter_info(arg2, "k"))
         return 1;

      boost::math::ntl::RR (*fp)(boost::math::ntl::RR, boost::math::ntl::RR) = &ellint_3;
      data.insert(fp, arg2, arg1);

      std::cout << "Any more data [y/n]?";
      std::getline(std::cin, line);
      boost::algorithm::trim(line);
      cont = (line == "y");
   }while(cont);

   std::cout << "Enter name of test data file [default=ellint_pi2_data.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "ellint_pi2_data.ipp";
   std::ofstream ofs(line.c_str());
   line.erase(line.find('.'));
   ofs << std::scientific;
   write_code(ofs, data, line.c_str());

   return 0;
}


