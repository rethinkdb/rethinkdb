/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_polymorphic2.cpp

// (C) Copyright 2009 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution
#include <fstream>

#include "test_tools.hpp"

#include "test_polymorphic2.hpp"

int test_main(int /*argc*/, char* /*argv*/[])
{
    A *a = new B();
    a->i = 3;

    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);
    {
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa_implementation(os, TEST_ARCHIVE_FLAGS);
        boost::archive::polymorphic_oarchive & opa = oa_implementation;
        opa << BOOST_SERIALIZATION_NVP(a);
    }
    A *loaded = 0;
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia_implementation(is, TEST_ARCHIVE_FLAGS);
        boost::archive::polymorphic_iarchive & ipa = ia_implementation;
        ipa >> BOOST_SERIALIZATION_NVP(loaded);
    }
    BOOST_CHECK(a->i == loaded->i);
    delete a;
    delete loaded;
    return EXIT_SUCCESS;
}
