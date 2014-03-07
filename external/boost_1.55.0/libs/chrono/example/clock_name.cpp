//  stopclock_perf.cpp  ---------------------------------------------------//

//  Copyright 2009 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.

#include "clock_name.hpp"
#include <iostream>

int main()
{
    std::cout << name<boost::chrono::system_clock>::apply() << '\n';
#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    std::cout << name<boost::chrono::steady_clock>::apply() << '\n';
#endif
    std::cout << name<boost::chrono::high_resolution_clock>::apply() << '\n';
}
