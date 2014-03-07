//  io_ex2.cpp  ----------------------------------------------------------//

//  Copyright 2010 Howard Hinnant
//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

/*
This code was adapted by Vicente J. Botet Escriba from Hinnant's html documentation.
Many thanks to Howard for making his code available under the Boost license.

*/

#include <boost/chrono/chrono_io.hpp>
#include <sstream>
#include <boost/assert.hpp>

int main()
{
    using namespace boost::chrono;

    std::istringstream in("5000 milliseconds 4000 ms 3001 ms");
    seconds d(0);
    in >> d;
    BOOST_ASSERT(in.good());
    BOOST_ASSERT(d == seconds(5));
    in >> d;
    BOOST_ASSERT(in.good());
    BOOST_ASSERT(d == seconds(4));
    in >> d;
    BOOST_ASSERT(in.fail());
    BOOST_ASSERT(d == seconds(4));
    return 0;
}

