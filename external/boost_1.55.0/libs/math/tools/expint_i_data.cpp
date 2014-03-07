//  (C) Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/bindings/rr.hpp>
#include <boost/math/special_functions/expint.hpp>
#include <boost/math/constants/constants.hpp>
#include <fstream>

#include <boost/math/tools/test_data.hpp>

using namespace boost::math::tools;


int main()
{
   boost::math::ntl::RR::SetPrecision(1000);
   boost::math::ntl::RR::SetOutputPrecision(40);

   parameter_info<boost::math::ntl::RR> arg1;
   test_data<boost::math::ntl::RR> data;

   boost::math::ntl::RR (*f)(boost::math::ntl::RR) = boost::math::expint;

   std::cout << "Welcome.\n"
      "This program will generate spot tests for the expint Ei function:\n"
      "  expint(a)\n\n";

   bool cont;
   std::string line;

   do{
      get_user_parameter_info(arg1, "a");
      data.insert(f, arg1);

      std::cout << "Any more data [y/n]?";
      std::getline(std::cin, line);
      boost::algorithm::trim(line);
      cont = (line == "y");
   }while(cont);

   std::cout << "Enter name of test data file [default=expinti_data.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "expinti_data.ipp";
   std::ofstream ofs(line.c_str());
   write_code(ofs, data, "expinti_data");
   
   return 0;
}

