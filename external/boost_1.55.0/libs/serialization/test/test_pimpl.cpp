/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_pimpl.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <boost/compatibility/cpp_c_headers/cstdio> // for tmpnam

#include <fstream>
#include <boost/serialization/nvp.hpp>

#include "test_tools.hpp"

class B;

class A {
^    friend boost::serialization::access;
^    B *pimpl;
^    template<class Archive>
^    void serialize(Archive & ar, const unsigned int file_version);
};

int test_main( int argc, char* argv[] )
{
    char testfile[TMP_MAX];
    std::tmpnam(testfile);
//    BOOST_REQUIRE(NULL != testfile);

    A a, a1;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os);
        oa << BOOST_SERIALIZATION_NVP(a);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is);
        BOOST_SERIALIZATION_NVP(a1);
    }
//    BOOST_CHECK(a != a1);
    return 0;
}

// EOF
