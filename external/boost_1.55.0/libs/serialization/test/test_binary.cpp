/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_simple_class.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <cstdlib> // for rand(), NULL, size_t

#include <fstream>
#include <boost/config.hpp>

#include <cstdio> // remove
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::rand; 
    using ::remove;
}
#endif

#include "test_tools.hpp"

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
        data[i] = static_cast<char>(0xff & std::rand());
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
    char s1[] = "a";
    char s2[] = "ab";
    char s3[] = "abc";
    char s4[] = "abcd";
    const int i = 12345;

    A a1;
    char s1_1[10];
    char s1_2[10];
    char s1_3[10];
    char s1_4[10];
    int i1 = 34790;

    std::memset(s1_1, '\0', sizeof(s1_1));
    std::memset(s1_2, '\0', sizeof(s1_2));
    std::memset(s1_3, '\0', sizeof(s1_3));
    std::memset(s1_4, '\0', sizeof(s1_4));
    {
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp(
            "s1", 
            boost::serialization::make_binary_object(
                s1, 
                sizeof(s1)
            )
        );
        oa << boost::serialization::make_nvp(
            "s2", 
            boost::serialization::make_binary_object(
                s2, 
                sizeof(s2)
            )
        );
        oa << boost::serialization::make_nvp(
            "s3", 
            boost::serialization::make_binary_object(
                s3, 
                sizeof(s3)
            )
        );
        oa << boost::serialization::make_nvp(
            "s4", 
            boost::serialization::make_binary_object(
                s4, 
                sizeof(s4)
            )
        );
        oa << BOOST_SERIALIZATION_NVP(a);
        // note: add a little bit on the end of the archive to detect
        // failure of text mode binary.
        oa << BOOST_SERIALIZATION_NVP(i);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp(
            "s1", 
            boost::serialization::make_binary_object(
                s1_1, 
                sizeof(s1)
            )
        );
        ia >> boost::serialization::make_nvp(
            "s2", 
            boost::serialization::make_binary_object(
                s1_2, 
                sizeof(s2)
            )
        );
        ia >> boost::serialization::make_nvp(
            "s3", 
            boost::serialization::make_binary_object(
                s1_3, 
                sizeof(s3)
            )
        );
        ia >> boost::serialization::make_nvp(
            "s4", 
            boost::serialization::make_binary_object(
                s1_4, 
                sizeof(s4)
            )
        );
        ia >> BOOST_SERIALIZATION_NVP(a1);
        // note: add a little bit on the end of the archive to detect
        // failure of text mode binary.
        ia >> BOOST_SERIALIZATION_NVP(i1);
    }
    BOOST_CHECK(0 == std::strcmp(s1, s1_1));
    BOOST_CHECK(0 == std::strcmp(s2, s1_2));
    BOOST_CHECK(0 == std::strcmp(s3, s1_3));
    BOOST_CHECK(0 == std::strcmp(s4, s1_4));
    BOOST_CHECK(a == a1);
    BOOST_CHECK(i == i1);
    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
