/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_polymorphic.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <cstddef> // NULL
#include <cstdio> // remove
#include <fstream>

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

// the following is to ensure that when one of the libraries changes
// BJAM rebuilds and relinks the test.
/*
#include "polymorphic_text_archive.hpp"
#include "polymorphic_text_warchive.hpp"
#include "polymorphic_binary_archive.hpp"
#include "polymorphic_xml_archive.hpp"
#include "polymorphic_xml_warchive.hpp"
*/

#include "test_tools.hpp"

#include <boost/archive/polymorphic_oarchive.hpp>
#include <boost/archive/polymorphic_iarchive.hpp>

#include <boost/serialization/nvp.hpp>
#include "test_polymorphic_A.hpp"

int test_main(int /* argc */, char * /* argv */ [])
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);
    const data d;
    data d1;
    // test using using polymorphic interface
    {
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa_implementation(os, TEST_ARCHIVE_FLAGS);
        boost::archive::polymorphic_oarchive & oa_interface = oa_implementation;
        oa_interface << BOOST_SERIALIZATION_NVP(d);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive  ia_implementation(is, TEST_ARCHIVE_FLAGS);
        boost::archive::polymorphic_iarchive & ia_interface = ia_implementation;
        ia_interface >> BOOST_SERIALIZATION_NVP(d1);
    }
    BOOST_CHECK(d == d1);
    std::remove(testfile);

    // test using using polymorphic implementation.
    {
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa_implementation(os, TEST_ARCHIVE_FLAGS);
        oa_implementation << BOOST_SERIALIZATION_NVP(d);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive  ia_implementation(is, TEST_ARCHIVE_FLAGS);
        ia_implementation >> BOOST_SERIALIZATION_NVP(d1);
    }
    BOOST_CHECK(d == d1);
    std::remove(testfile);

    // test using using polymorphic implementation.
    {
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        boost::archive::polymorphic_oarchive * oa_implementation 
            = new test_oarchive(os, TEST_ARCHIVE_FLAGS);
        *oa_implementation << BOOST_SERIALIZATION_NVP(d);
        delete oa_implementation;
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        boost::archive::polymorphic_iarchive * ia_implementation
            = new test_iarchive(is, TEST_ARCHIVE_FLAGS);
        *ia_implementation >> BOOST_SERIALIZATION_NVP(d1);
        delete ia_implementation;
    }
    BOOST_CHECK(d == d1);
    std::remove(testfile);
    return EXIT_SUCCESS;
}
