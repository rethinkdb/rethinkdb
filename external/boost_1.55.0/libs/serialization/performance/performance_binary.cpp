/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_simple_class.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <fstream>

#include <cstdlib> // for rand()
#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::rand; 
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
#include <boost/serialization/binary_object.hpp>

class A {
    friend class boost::serialization::access;
    char data[150];
    // note: from an aesthetic perspective, I would much prefer to have this
    // defined out of line.  Unfortunately, this trips a bug in the VC 6.0
    // compiler. So hold our nose and put it her to permit running of tests.
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & boost::serialization::make_nvp(
            "data",
            boost::serialization::make_binary_object(data, sizeof(data))
        );
    }

public:
    A();
    bool operator==(const A & rhs) const;
};

A::A(){
    int i = sizeof(data);
    while(i-- > 0)
        data[i] = 0xff & std::rand();
}

bool A::operator==(const A & rhs) const {
    int i = sizeof(data);
    while(i-- > 0)
        if(data[i] != rhs.data[i])
            return false;
    return true;
}

int test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    const A a;
    A a1;
    const int i = 12345;
    int i1 = 34790;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << BOOST_SERIALIZATION_NVP(a);
        // note: add a little bit on the end of the archive to detect
        // failure of text mode binary.
        oa << BOOST_SERIALIZATION_NVP(i);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> BOOST_SERIALIZATION_NVP(a1);
        ia >> BOOST_SERIALIZATION_NVP(i1);
    }
    BOOST_CHECK(i == i1);
    BOOST_CHECK(a == a1);
    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
