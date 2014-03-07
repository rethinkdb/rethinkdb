//  (C) Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/bindings/rr.hpp>
#include <boost/limits.hpp>
#include <vector>

void write_table(unsigned max_exponent)
{
   boost::math::ntl::RR max = ldexp(boost::math::ntl::RR(1), max_exponent);

   std::vector<boost::math::ntl::RR> factorials;
   factorials.push_back(1);

   boost::math::ntl::RR f(1);
   unsigned i = 1;

   while(f < max)
   {
      factorials.push_back(f);
      ++i;
      f *= i;
   }

   //
   // now write out the results to cout:
   //
   std::cout << std::scientific;
   std::cout << "   static const boost::array<T, " << factorials.size() << "> factorials = {\n";
   for(unsigned j = 0; j < factorials.size(); ++j)
      std::cout << "      " << factorials[j] << "L,\n";
   std::cout << "   };\n\n";
}


int main()
{
   boost::math::ntl::RR::SetPrecision(300);
   boost::math::ntl::RR::SetOutputPrecision(40);
   write_table(16384/*std::numeric_limits<float>::max_exponent*/);
}
