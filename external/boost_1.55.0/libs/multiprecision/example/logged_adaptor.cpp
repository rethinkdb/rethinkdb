///////////////////////////////////////////////////////////////
//  Copyright 2013 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

//[logged_adaptor

#include <boost/multiprecision/mpfi.hpp>
#include <boost/multiprecision/logged_adaptor.hpp>
#include <iostream>
#include <iomanip>
//
// Begin by overloading log_postfix_event so we can capture each arithmetic event as it happens:
//
namespace boost{ namespace multiprecision{

template <unsigned D>
inline void log_postfix_event(const mpfi_float_backend<D>& val, const char* event_description)
{
   // Print out the (relative) diameter of the interval:
   using namespace boost::multiprecision;
   number<mpfr_float_backend<D> > diam;
   mpfi_diam(diam.backend().data(), val.data());
   std::cout << "Diameter was " << diam << " after operation: " << event_description << std::endl;
}
template <unsigned D, class T>
inline void log_postfix_event(const mpfi_float_backend<D>&, const T&, const char* event_description)
{
   // This version is never called in this example.
}

}}


int main()
{
   using namespace boost::multiprecision;
   typedef number<logged_adaptor<mpfi_float_backend<17> > > logged_type;
   //
   // Test case deliberately introduces cancellation error, relative size of interval
   // gradually gets larger after each operation:
   //
   logged_type a = 1;
   a /= 10;

   for(unsigned i = 0; i < 13; ++i)
   {
      logged_type b = a * 9;
      b /= 10;
      a -= b;
   }
   std::cout << "Final value was: " << a << std::endl;
   return 0;
}

//]

/*
//[logged_adaptor_output

Diameter was nan after operation: Default construct
Diameter was 0 after operation: Assignment from arithmetic type
Diameter was 4.33681e-18 after operation: /=
Diameter was nan after operation: Default construct
Diameter was 7.70988e-18 after operation: *
Diameter was 9.63735e-18 after operation: /=
Diameter was 1.30104e-16 after operation: -=
Diameter was nan after operation: Default construct
Diameter was 1.30104e-16 after operation: *
Diameter was 1.38537e-16 after operation: /=
Diameter was 2.54788e-15 after operation: -=
Diameter was nan after operation: Default construct
Diameter was 2.54788e-15 after operation: *
Diameter was 2.54863e-15 after operation: /=
Diameter was 4.84164e-14 after operation: -=
Diameter was nan after operation: Default construct
Diameter was 4.84164e-14 after operation: *
Diameter was 4.84221e-14 after operation: /=
Diameter was 9.19962e-13 after operation: -=
Diameter was nan after operation: Default construct
Diameter was 9.19962e-13 after operation: *
Diameter was 9.19966e-13 after operation: /=
Diameter was 1.74793e-11 after operation: -=
Diameter was nan after operation: Default construct
Diameter was 1.74793e-11 after operation: *
Diameter was 1.74793e-11 after operation: /=
Diameter was 3.32107e-10 after operation: -=
Diameter was nan after operation: Default construct
Diameter was 3.32107e-10 after operation: *
Diameter was 3.32107e-10 after operation: /=
Diameter was 6.31003e-09 after operation: -=
Diameter was nan after operation: Default construct
Diameter was 6.31003e-09 after operation: *
Diameter was 6.31003e-09 after operation: /=
Diameter was 1.19891e-07 after operation: -=
Diameter was nan after operation: Default construct
Diameter was 1.19891e-07 after operation: *
Diameter was 1.19891e-07 after operation: /=
Diameter was 2.27792e-06 after operation: -=
Diameter was nan after operation: Default construct
Diameter was 2.27792e-06 after operation: *
Diameter was 2.27792e-06 after operation: /=
Diameter was 4.32805e-05 after operation: -=
Diameter was nan after operation: Default construct
Diameter was 4.32805e-05 after operation: *
Diameter was 4.32805e-05 after operation: /=
Diameter was 0.00082233 after operation: -=
Diameter was nan after operation: Default construct
Diameter was 0.00082233 after operation: *
Diameter was 0.00082233 after operation: /=
Diameter was 0.0156243 after operation: -=
Diameter was nan after operation: Default construct
Diameter was 0.0156243 after operation: *
Diameter was 0.0156243 after operation: /=
Diameter was 0.296861 after operation: -=
Final value was: {8.51569e-15,1.14843e-14}

//]
*/
