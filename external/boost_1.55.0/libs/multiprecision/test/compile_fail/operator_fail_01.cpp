///////////////////////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/multiprecision/cpp_dec_float.hpp>

using namespace boost::multiprecision;

int main()
{
   number<cpp_dec_float<50>, et_on>  a(2);
   number<cpp_dec_float<50>, et_off> b(2);

   a = a + b;
}

