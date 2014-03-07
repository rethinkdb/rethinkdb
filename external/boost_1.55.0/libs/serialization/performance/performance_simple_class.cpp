/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_simple_class.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

// invoke header for a custom archive test.

#include <fstream>

#include <cstdio> // remove
#include <boost/config.hpp>

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include "../test/test_tools.hpp"
#include <boost/preprocessor/stringize.hpp>
// #include <boost/preprocessor/cat.hpp>
// the following fails with (only!) gcc 3.4 
// #include BOOST_PP_STRINGIZE(BOOST_PP_CAT(../test/,BOOST_ARCHIVE_TEST))
// just copy over the files from the test directory
#include BOOST_PP_STRINGIZE(BOOST_ARCHIVE_TEST)

#include <boost/serialization/nvp.hpp>
#include "../test/A.hpp"

int 
test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    A a, a1;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("a", a);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("a", a1);
    }
    BOOST_CHECK_EQUAL(a, a1);
    std::remove(testfile);
    return (a == a1) ? EXIT_SUCCESS : EXIT_SUCCESS;
}
