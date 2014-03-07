/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// demo_polymorphic.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <sstream>

#include <boost/archive/polymorphic_text_iarchive.hpp>
#include <boost/archive/polymorphic_text_oarchive.hpp>

#include <boost/archive/polymorphic_binary_iarchive.hpp>
#include <boost/archive/polymorphic_binary_oarchive.hpp>

#include "demo_polymorphic_A.hpp"

int main(int argc, char* argv[])
{
    const A a;
    A a1;
    {
        // test with a text archive
        std::stringstream ss;
        {
            // instantiate archive which inhertis polymorphic interface
            // and the normal text archive implementation
            boost::archive::polymorphic_text_oarchive oa(ss);
            boost::archive::polymorphic_oarchive & oa_interface = oa; 
            // we can just just the interface for saving
            oa_interface << a;
        }
        {
            // or we can use the implementation directly
            boost::archive::polymorphic_text_iarchive ia(ss);
            ia >> a1;
        }
    }
    if(! (a == a1))
        return 1;
    {
        //test with a binary archive
        std::stringstream ss;
        {
            // instantiate archive which inhertis polymorphic interface
            // and the normal binary archive implementation
            boost::archive::polymorphic_binary_oarchive oa(ss);
            oa << a;
        }
        {
            // see above
            boost::archive::polymorphic_binary_iarchive ia(ss);
            boost::archive::polymorphic_iarchive & ia_interface = ia; 
            // use just the polymorphic interface for loading.
            ia_interface >> a1;
        }
    }
    if(! (a == a1))
        return 1;
    return 0;
}

