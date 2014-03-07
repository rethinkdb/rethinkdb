//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef NTL_STD_CXX
#  define NTL_STD_CXX
#endif

#include <NTL/RR.h>
#include <iostream>
#include <iomanip>

int main()
{
   NTL::RR r, root_two;
   r.SetPrecision(256);
   root_two.SetPrecision(256);
   r = 1.0;
   root_two = 2.0;
   root_two = NTL::sqrt(root_two);
   r /= root_two;
   NTL::RR lim = NTL::pow(NTL::RR(NTL::INIT_VAL, 2.0), NTL::RR(NTL::INIT_VAL, -128));
   NTL::RR::SetOutputPrecision(40);
   while(r > lim)
   {
      std::cout << "   { " << r << "L, " << NTL::log1p(r) << "L, " << NTL::expm1(r) << "L, }, \n";
      std::cout << "   { " << -r << "L, " << NTL::log1p(-r) << "L, " << NTL::expm1(-r) << "L, }, \n";
      r /= root_two;
   }
   return 0;
}

