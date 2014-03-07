//  (C) Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/bindings/rr.hpp>
#include <boost/math/tools/test_data.hpp>
#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/math/special_functions/hermite.hpp>
#include <fstream>
#include <boost/math/tools/test_data.hpp>
#include <boost/tr1/random.hpp>

using namespace boost::math::tools;
using namespace boost::math;
using namespace std;


template<class T>
boost::math::tuple<T, T, T> hermite_data(T n, T x)
{
   n = floor(n);
   T r1 = hermite(boost::math::tools::real_cast<unsigned>(n), x);
   return boost::math::make_tuple(n, x, r1);
}
   
int cpp_main(int argc, char*argv [])
{
   using namespace boost::math::tools;

   boost::math::ntl::RR::SetOutputPrecision(50);
   boost::math::ntl::RR::SetPrecision(1000);

   parameter_info<boost::math::ntl::RR> arg1, arg2, arg3;
   test_data<boost::math::ntl::RR> data;

   std::cout << boost::math::hermite(10, static_cast<boost::math::ntl::RR>(1e300)) << std::endl;

   bool cont;
   std::string line;

   if(argc < 1)
      return 1;

   do{
      if(0 == get_user_parameter_info(arg1, "n"))
         return 1;
      if(0 == get_user_parameter_info(arg2, "x"))
         return 1;
      arg1.type |= dummy_param;
      arg2.type |= dummy_param;

      data.insert(&hermite_data<boost::math::ntl::RR>, arg1, arg2);

      std::cout << "Any more data [y/n]?";
      std::getline(std::cin, line);
      boost::algorithm::trim(line);
      cont = (line == "y");
   }while(cont);

   std::cout << "Enter name of test data file [default=hermite.ipp]";
   std::getline(std::cin, line);
   boost::algorithm::trim(line);
   if(line == "")
      line = "hermite.ipp";
   std::ofstream ofs(line.c_str());
   line.erase(line.find('.'));
   ofs << std::scientific;
   write_code(ofs, data, line.c_str());

   return 0;
}

