//  io_ex1.cpp  ----------------------------------------------------------//

//  Copyright 2010 Howard Hinnant
//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

/*
This code was adapted by Vicente J. Botet Escriba from Hinnant's html documentation.
Many thanks to Howard for making his code available under the Boost license.

*/

#include <boost/chrono/chrono_io.hpp>
#include <iostream>

int main()
{
    using std::cout;
    using namespace boost;
    using namespace boost::chrono;

#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    typedef time_point<steady_clock, duration<double, ratio<3600> > > T;
    T tp = steady_clock::now();
    std::cout << tp << '\n';
#endif
    return 0;
}

//~ 17.8666 hours since boot
