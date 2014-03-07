// (C) Copyright Jeremy Siek 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



// An example of setting and reading some bits. Note that operator[]
// goes from the least-significant bit at 0 to the most significant
// bit at size()-1. The operator<< for dynamic_bitset prints the
// bitset from most-significant to least-significant, since that is
// the format most people are used to reading.
//
//  The output is:
//
//  11001
//  10011
// ---------------------------------------------------------------------

#include <iostream>
#include <boost/dynamic_bitset.hpp>

int main()
{
    boost::dynamic_bitset<> x(5); // all 0's by default
    x[0] = 1;
    x[1] = 1;
    x[4] = 1;
    for (boost::dynamic_bitset<>::size_type i = 0; i < x.size(); ++i)
        std::cout << x[i];
    std::cout << "\n";
    std::cout << x << "\n";

    return 0;
}
