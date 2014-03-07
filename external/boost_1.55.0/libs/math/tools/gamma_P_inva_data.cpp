//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/bindings/rr.hpp>
#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/test.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>

#include <boost/math/tools/test_data.hpp>

using namespace boost::math::tools;

//
// Force trunctation to float precision of input values:
// we must ensure that the input values are exactly representable
// in whatever type we are testing, or the output values will all
// be thrown off:
//
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

struct gamma_inverse_generator_a
{
   boost::math::tuple<boost::math::ntl::RR, boost::math::ntl::RR> operator()(const boost::math::ntl::RR x, const boost::math::ntl::RR p)
   {
      boost::math::ntl::RR x1 = boost::math::gamma_p_inva(x, p);
      boost::math::ntl::RR x2 = boost::math::gamma_q_inva(x, p);
      std::cout << "Inverse for " << x << " " << p << std::endl;
      return boost::math::make_tuple(x1, x2);
   }
};


int cpp_main(int argc, char*argv [])
{
   boost::math::ntl::RR::SetPrecision(1000);
   boost::math::ntl::RR::SetOutputPrecision(100);

   bool cont;
   std::string line;

   parameter_info<boost::math::ntl::RR> arg1, arg2;
   test_data<boost::math::ntl::RR> data;

   std::cout << "Welcome.\n"
      "This program will generate spot tests for the inverse incomplete gamma function:\n"
      "  gamma_p_inva(a, p) and gamma_q_inva(a, q)\n\n";

   arg1 = make_power_param<boost::math::ntl::RR>(boost::math::ntl::RR(0), -4, 24);
   arg2 = make_random_param<boost::math::ntl::RR>(boost::math::ntl::RR(0), boost::math::ntl::RR(1), 15);
   data.insert(gamma_inverse_generator_a(), arg1, arg2);
 
   line = "igamma_inva_data.ipp";
   std::ofstream ofs(line.c_str());
   write_code(ofs, data, "igamma_inva_data");
   
   return 0;
}

