//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/bindings/rr.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/test_data.hpp>
#include <fstream>

using namespace boost::math::tools;

struct beta_data_generator
{
   boost::math::ntl::RR operator()(boost::math::ntl::RR a, boost::math::ntl::RR b)
   {
      if(a < b)
         throw std::domain_error("");
      // very naively calculate spots:
      boost::math::ntl::RR g1, g2, g3;
      int s1, s2, s3;
      g1 = boost::math::lgamma(a, &s1);
      g2 = boost::math::lgamma(b, &s2);
      g3 = boost::math::lgamma(a+b, &s3);
      g1 += g2 - g3;
      g1 = exp(g1);
      g1 *= s1 * s2 * s3;
      return g1;
   }
};


int main()
{
   boost::math::ntl::RR::SetPrecision(1000);
   boost::math::ntl::RR::SetOutputPrecision(40);

   parameter_info<boost::math::ntl::RR> arg1, arg2;
   test_data<boost::math::ntl::RR> data;

   std::cout << "Welcome.\n"
      "This program will generate spot tests for the beta function:\n"
      "  beta(a, b)\n\n";

   bool cont;
   std::string line;

   do{
      get_user_parameter_info(arg1, "a");
      get_user_parameter_info(arg2, "b");
      data.insert(beta_data_generator(), arg1, arg2);

      std::cout << "Any more data [y/n]?";
      std::getline(std::cin, line);
      boost::algorithm::trim(line);
      cont = (line == "y");
   }while(cont);

   std::cout << "Enter name of test data file [default=beta_data.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "beta_data.ipp";
   std::ofstream ofs(line.c_str());
   write_code(ofs, data, "beta_data");
   
   return 0;
}

