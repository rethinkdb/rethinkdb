///////////////////////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/multiprecision/cpp_dec_float.hpp>

using namespace boost::multiprecision;

int main()
{
   cpp_dec_float_100 f(2);
   cpp_dec_float_50 f2;
   f2 = f;
}

