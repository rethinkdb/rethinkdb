/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_vector.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

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
#include <boost/serialization/vector.hpp>

#include "../test/A.hpp"

template <class T>
int test_vector(T)
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    // test array of objects
    std::vector<T> avector;
    avector.push_back(T());
    avector.push_back(T());
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("avector", avector);
    }
    std::vector<T> avector1;
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("avector", avector1);
    }
    BOOST_CHECK(avector == avector1);
    std::remove(testfile);
    return EXIT_SUCCESS;
}

int test_main( int /* argc */, char* /* argv */[] )
{
   int res = test_vector(A());
    // test an int vector for which optimized versions should be available
   if (res == EXIT_SUCCESS)
     res = test_vector(0);  
    // test a bool vector
   if (res == EXIT_SUCCESS)
     res = test_vector(false);  
   return res;
}

// EOF
