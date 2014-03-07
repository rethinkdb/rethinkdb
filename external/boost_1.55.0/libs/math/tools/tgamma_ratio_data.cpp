//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/bindings/rr.hpp>
#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/tools/test.hpp>
#include <fstream>

#include <boost/math/tools/test_data.hpp>

using namespace boost::math::tools;
using namespace std;

boost::math::tuple<boost::math::ntl::RR, boost::math::ntl::RR> 
   tgamma_ratio(const boost::math::ntl::RR& a, const boost::math::ntl::RR& delta)
{
   if(delta > a)
      throw std::domain_error("");
   boost::math::ntl::RR tg = boost::math::tgamma(a);
   boost::math::ntl::RR r1 = tg / boost::math::tgamma(a + delta);
   boost::math::ntl::RR r2 = tg / boost::math::tgamma(a - delta);
   if((r1 > (std::numeric_limits<float>::max)()) || (r2 > (std::numeric_limits<float>::max)()))
      throw std::domain_error("");

   return boost::math::make_tuple(r1, r2);
}

boost::math::ntl::RR tgamma_ratio2(const boost::math::ntl::RR& a, const boost::math::ntl::RR& b)
{
   return boost::math::tgamma(a) / boost::math::tgamma(b);
}


int cpp_main(int argc, char*argv [])
{
   boost::math::ntl::RR::SetPrecision(1000);
   boost::math::ntl::RR::SetOutputPrecision(40);

   parameter_info<boost::math::ntl::RR> arg1, arg2;
   test_data<boost::math::ntl::RR> data;

   bool cont;
   std::string line;

   if((argc >= 2) && (strcmp(argv[1], "--ratio") == 0))
   {
      std::cout << "Welcome.\n"
         "This program will generate spot tests for the function tgamma_ratio(a, b)\n\n";

      do{
         if(0 == get_user_parameter_info(arg1, "a"))
            return 1;
         if(0 == get_user_parameter_info(arg2, "b"))
            return 1;
         data.insert(&tgamma_ratio2, arg1, arg2);

         std::cout << "Any more data [y/n]?";
         std::getline(std::cin, line);
         boost::algorithm::trim(line);
         cont = (line == "y");
      }while(cont);
   }
   else
   {
      std::cout << "Welcome.\n"
         "This program will generate spot tests for the function tgamma_delta_ratio(a, delta)\n\n";

      do{
         if(0 == get_user_parameter_info(arg1, "a"))
            return 1;
         if(0 == get_user_parameter_info(arg2, "delta"))
            return 1;
         data.insert(&tgamma_ratio, arg1, arg2);

         std::cout << "Any more data [y/n]?";
         std::getline(std::cin, line);
         boost::algorithm::trim(line);
         cont = (line == "y");
      }while(cont);
   }

   std::cout << "Enter name of test data file [default=tgamma_ratio_data.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "tgamma_ratio_data.ipp";
   std::ofstream ofs(line.c_str());
   ofs << std::scientific;
   write_code(ofs, data, "tgamma_ratio_data");
   
   return 0;
}


