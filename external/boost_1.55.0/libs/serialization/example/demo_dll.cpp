/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_derived_class.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <cstddef> // NULL
#include <fstream>

#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include "test_tools.hpp"

#include "B.hpp"
#include "A.ipp"

int test_main( int argc, char* argv[] )
{
  const char * testfile = boost::archive::tmpnam(NULL);
  
  BOOST_REQUIRE(NULL != testfile);

    B b, b1;

    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("b", b);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("b1", b1);
    }
    BOOST_CHECK(b == b1);
    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
