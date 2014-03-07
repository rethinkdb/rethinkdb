/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// demo_pimpl.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <sstream>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "demo_pimpl_A.hpp"

int main(int argc, char* argv[])
{
    std::stringstream ss;

    const A a;
    {
        boost::archive::text_oarchive oa(ss);
        oa << a;
    }
    A a1;
    {
        boost::archive::text_iarchive ia(ss);
        ia >> a1;
    }
    return 0;
}

