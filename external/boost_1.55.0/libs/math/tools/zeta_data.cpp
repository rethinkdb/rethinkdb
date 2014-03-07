//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/bindings/rr.hpp>
#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/math/special_functions/zeta.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/test.hpp>
#include <fstream>

#include <boost/math/tools/test_data.hpp>

using namespace boost::math::tools;
using namespace std;

struct zeta_data_generator
{
   boost::math::ntl::RR operator()(boost::math::ntl::RR z)
   {
      std::cout << z << " ";
      boost::math::ntl::RR result = boost::math::zeta(z);
      std::cout << result << std::endl;
      return result;
   }
};

struct zeta_data_generator2
{
   boost::math::tuple<boost::math::ntl::RR, boost::math::ntl::RR> operator()(boost::math::ntl::RR z)
   {
      std::cout << -z << " ";
      boost::math::ntl::RR result = boost::math::zeta(-z);
      std::cout << result << std::endl;
      return boost::math::make_tuple(-z, result);
   }
};


int cpp_main(int argc, char*argv [])
{
   boost::math::ntl::RR::SetPrecision(500);
   boost::math::ntl::RR::SetOutputPrecision(40);

   parameter_info<boost::math::ntl::RR> arg1;
   test_data<boost::math::ntl::RR> data;

   bool cont;
   std::string line;

   std::cout << "Welcome.\n"
      "This program will generate spot tests for the zeta function:\n";

   do{
      if(0 == get_user_parameter_info(arg1, "z"))
         return 1;
      arg1.type |= dummy_param;
      data.insert(zeta_data_generator2(), arg1);

      std::cout << "Any more data [y/n]?";
      std::getline(std::cin, line);
      boost::algorithm::trim(line);
      cont = (line == "y");
   }while(cont);

   std::cout << "Enter name of test data file [default=zeta_data.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "zeta_data.ipp";
   std::ofstream ofs(line.c_str());
   write_code(ofs, data, "zeta_data");
   
   return 0;
}

