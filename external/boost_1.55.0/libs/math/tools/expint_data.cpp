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

struct expint_data_generator
{
   boost::math::ntl::RR operator()(boost::math::ntl::RR a, boost::math::ntl::RR b)
   {
      unsigned n = boost::math::tools::real_cast<unsigned>(a);
      std::cout << n << "  " << b << "  ";
      boost::math::ntl::RR result = boost::math::expint(n, b);
      std::cout << result << std::endl;
      return result;
   }
};


int main()
{
   boost::math::ntl::RR::SetPrecision(1000);
   boost::math::ntl::RR::SetOutputPrecision(40);

   boost::math::expint(1, 0.06227754056453704833984375);
   std::cout << boost::math::expint(1, boost::math::ntl::RR(0.5)) << std::endl;

   parameter_info<boost::math::ntl::RR> arg1, arg2;
   test_data<boost::math::ntl::RR> data;

   std::cout << "Welcome.\n"
      "This program will generate spot tests for the expint function:\n"
      "  expint(a, b)\n\n";

   bool cont;
   std::string line;

   do{
      get_user_parameter_info(arg1, "a");
      get_user_parameter_info(arg2, "b");
      data.insert(expint_data_generator(), arg1, arg2);

      std::cout << "Any more data [y/n]?";
      std::getline(std::cin, line);
      boost::algorithm::trim(line);
      cont = (line == "y");
   }while(cont);

   std::cout << "Enter name of test data file [default=expint_data.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "expint_data.ipp";
   std::ofstream ofs(line.c_str());
   write_code(ofs, data, "expint_data");
   
   return 0;
}

