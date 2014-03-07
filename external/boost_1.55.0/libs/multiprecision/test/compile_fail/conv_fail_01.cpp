///////////////////////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/multiprecision/cpp_int.hpp>

using namespace boost::multiprecision;

void foo(cpp_int i)
{
}

int main()
{
   foo(2.3); // conversion from float is explicit
}
