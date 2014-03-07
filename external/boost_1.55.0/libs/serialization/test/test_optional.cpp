/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_optional.cpp

// (C) Copyright 2004 Pavel Vozenilek
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


#include <boost/archive/archive_exception.hpp>
#include "test_tools.hpp"

#include <boost/serialization/optional.hpp>

#include "A.hpp"
#include "A.ipp"

int test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    const boost::optional<int> aoptional1;
    const boost::optional<int> aoptional2(123);
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("aoptional1",aoptional1);
        oa << boost::serialization::make_nvp("aoptional2",aoptional2);
    }

    boost::optional<int> aoptional1a(999);
    boost::optional<int> aoptional2a;

    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("aoptional1",aoptional1a);
        ia >> boost::serialization::make_nvp("aoptional2",aoptional2a);
    }
    BOOST_CHECK(aoptional1 == aoptional1a);
    BOOST_CHECK(aoptional2 == aoptional2a);
    
    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
