//  (C) Copyright John Maddock 2009.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//#define BOOST_MATH_INSTRUMENT

#include <boost/math/bindings/rr.hpp>
#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/math/distributions/hypergeometric.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/test.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random/uniform_int.hpp>
#include <fstream>

#include <boost/math/tools/test_data.hpp>

using namespace boost::math::tools;

std::tr1::mt19937 rnd;

struct hypergeometric_generator
{
   boost::math::tuple<
      boost::math::ntl::RR, 
      boost::math::ntl::RR, 
      boost::math::ntl::RR, 
      boost::math::ntl::RR, 
      boost::math::ntl::RR,
      boost::math::ntl::RR,
      boost::math::ntl::RR> operator()(boost::math::ntl::RR rN, boost::math::ntl::RR rr, boost::math::ntl::RR rn)
   {
      using namespace std;
      using namespace boost;
      using namespace boost::math;

      if((rr > rN) || (rr < rn))
         throw std::domain_error("");

      try{
         int N = itrunc(rN);
         int r = itrunc(rr);
         int n = itrunc(rn);
         boost::uniform_int<> ui((std::max)(0, n + r - N), (std::min)(n, r));
         int k = ui(rnd);

         hypergeometric_distribution<ntl::RR> d(r, n, N);

         ntl::RR p = pdf(d, k);
         if((p == 1) || (p == 0))
         {
            // trivial case, don't clutter up our table with it:
            throw std::domain_error("");
         }
         ntl::RR c = cdf(d, k);
         ntl::RR cc = cdf(complement(d, k));

         std::cout << "N = " << N << " r = " << r << " n = " << n << " PDF = " << p << " CDF = " << c << " CCDF = " << cc << std::endl;

         return boost::math::make_tuple(r, n, N, k, p, c, cc);
      }
      catch(const std::exception& e)
      {
         std::cout << e.what() << std::endl;
         throw std::domain_error("");
      }
   }
};

int cpp_main(int argc, char*argv [])
{
   boost::math::ntl::RR::SetPrecision(1000);
   boost::math::ntl::RR::SetOutputPrecision(100);

   std::string line;
   parameter_info<boost::math::ntl::RR> arg1, arg2, arg3;
   test_data<boost::math::ntl::RR> data;

   std::cout << "Welcome.\n"
      "This program will generate spot tests hypergeoemtric distribution:\n";

   arg1 = make_power_param(boost::math::ntl::RR(0), 1, 21);
   arg2 = make_power_param(boost::math::ntl::RR(0), 1, 21);
   arg3 = make_power_param(boost::math::ntl::RR(0), 1, 21);

   arg1.type |= dummy_param;
   arg2.type |= dummy_param;
   arg3.type |= dummy_param;

   data.insert(hypergeometric_generator(), arg1, arg2, arg3);

   line = "hypergeometric_dist_data2.ipp";
   std::ofstream ofs(line.c_str());
   write_code(ofs, data, "hypergeometric_dist_data2");
   
   return 0;
}

