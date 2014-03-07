
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "hash_float_test.hpp"

int main()
{
    std::cerr<<"Compiler: "<<BOOST_COMPILER<<"\n";
    std::cerr<<"Platform: "<<BOOST_PLATFORM<<"\n";
    std::cerr<<"Library: "<<BOOST_STDLIB<<"\n\n";

    float_tests("float", (float*) 0);
    float_tests("double", (double*) 0);

    return boost::report_errors();
}
