/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_contained_class.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <cstddef>
#include <fstream>

#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include "test_tools.hpp"
#include <boost/serialization/nvp.hpp>

#include "B.hpp"
#include "A.ipp"

///////////////////////////////////////////////////////
// Contained class
class C
{
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_NVP(b);
    }
    B b;
public:
    bool operator==(const C &rhs) const
    {
        return b == rhs.b;
    }
    C(){}
};

int test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    const C c;
    C c1;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("c", c);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("c", c1);
    }
    BOOST_CHECK(c == c1);
    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
