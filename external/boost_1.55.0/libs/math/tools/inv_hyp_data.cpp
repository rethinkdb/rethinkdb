//  Copyright John Maddock 2008.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/bindings/rr.hpp>
#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/test.hpp>
#include <fstream>

#include <boost/math/tools/test_data.hpp>

using namespace boost::math::tools;
using namespace std;

struct asinh_data_generator
{
   boost::math::ntl::RR operator()(boost::math::ntl::RR z)
   {
      std::cout << z << " ";
      boost::math::ntl::RR result = log(z + sqrt(z * z + 1));
      std::cout << result << std::endl;
      return result;
   }
};

struct acosh_data_generator
{
   boost::math::ntl::RR operator()(boost::math::ntl::RR z)
   {
      std::cout << z << " ";
      boost::math::ntl::RR result = log(z + sqrt(z * z - 1));
      std::cout << result << std::endl;
      return result;
   }
};

struct atanh_data_generator
{
   boost::math::ntl::RR operator()(boost::math::ntl::RR z)
   {
      std::cout << z << " ";
      boost::math::ntl::RR result = log((z + 1) / (1 - z)) / 2;
      std::cout << result << std::endl;
      return result;
   }
};

int cpp_main(int argc, char*argv [])
{
   boost::math::ntl::RR::SetPrecision(500);
   boost::math::ntl::RR::SetOutputPrecision(40);

   parameter_info<boost::math::ntl::RR> arg1;
   test_data<boost::math::ntl::RR> data;
   std::ofstream ofs;

   bool cont;
   std::string line;

   std::cout << "Welcome.\n"
      "This program will generate spot tests for the inverse hyperbolic sin function:\n";

   do{
      if(0 == get_user_parameter_info(arg1, "z"))
         return 1;
      data.insert(asinh_data_generator(), arg1);

      std::cout << "Any more data [y/n]?";
      std::getline(std::cin, line);
      boost::algorithm::trim(line);
      cont = (line == "y");
   }while(cont);

   std::cout << "Enter name of test data file [default=asinh_data.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "asinh_data.ipp";
   ofs.open(line.c_str());
   write_code(ofs, data, "asinh_data");
   data.clear();

   std::cout << "Welcome.\n"
      "This program will generate spot tests for the inverse hyperbolic cos function:\n";

   do{
      if(0 == get_user_parameter_info(arg1, "z"))
         return 1;
      data.insert(acosh_data_generator(), arg1);

      std::cout << "Any more data [y/n]?";
      std::getline(std::cin, line);
      boost::algorithm::trim(line);
      cont = (line == "y");
   }while(cont);

   std::cout << "Enter name of test data file [default=acosh_data.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "acosh_data.ipp";
   ofs.close();
   ofs.open(line.c_str());
   write_code(ofs, data, "acosh_data");
   data.clear();

   std::cout << "Welcome.\n"
      "This program will generate spot tests for the inverse hyperbolic tan function:\n";

   do{
      if(0 == get_user_parameter_info(arg1, "z"))
         return 1;
      data.insert(atanh_data_generator(), arg1);

      std::cout << "Any more data [y/n]?";
      std::getline(std::cin, line);
      boost::algorithm::trim(line);
      cont = (line == "y");
   }while(cont);

   std::cout << "Enter name of test data file [default=atanh_data.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "atanh_data.ipp";
   ofs.close();
   ofs.open(line.c_str());
   write_code(ofs, data, "atanh_data");
   
   return 0;
}

