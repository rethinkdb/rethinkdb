/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_class_info_save.cpp: test implementation level trait

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// test implementation level "object_class_info"
// should pass compilation and execution

#include <fstream>
#include <string>

#include <boost/static_assert.hpp>
#include <boost/archive/tmpdir.hpp>
#include <boost/preprocessor/stringize.hpp>
#include "test_tools.hpp"

#include <boost/serialization/level.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/nvp.hpp>

// first case : serialize WITHOUT class information
class A
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & /*ar*/, const unsigned int file_version){
        // class that don't save class info always have a version number of 0
        BOOST_CHECK(file_version == 0);
        BOOST_STATIC_ASSERT(0 == ::boost::serialization::version<A>::value);
        ++count;
    }
public:
    unsigned int count;
    A() : count(0) {}
};

BOOST_CLASS_IMPLEMENTATION(A, ::boost::serialization::object_serializable)
BOOST_CLASS_TRACKING(A, ::boost::serialization::track_never)

// second case : serialize WITH class information
// note: GCC compile fails if this is after the class declaration
class B;
BOOST_CLASS_VERSION(B, 2)

class B
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & /*ar*/, const unsigned int file_version){
        // verify at execution that correct version number is passed on save
        BOOST_CHECK(
            static_cast<const int>(file_version) 
            == ::boost::serialization::version<B>::value
        );
        ++count;
    }
public:
    unsigned int count;
    B() : count(0) {}
};

BOOST_CLASS_IMPLEMENTATION(B, ::boost::serialization::object_class_info)
BOOST_CLASS_TRACKING(B, boost::serialization::track_always)

#include <iostream>

void out(const char *testfile, const A & a, const B & b)
{
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
    // write object twice to check tracking
    oa << BOOST_SERIALIZATION_NVP(a);
    oa << BOOST_SERIALIZATION_NVP(a);
    BOOST_CHECK(a.count == 2);  // no tracking => redundant saves
    std::cout << "a.count=" << a.count << '\n' ;
    oa << BOOST_SERIALIZATION_NVP(b);
    oa << BOOST_SERIALIZATION_NVP(b);
    BOOST_CHECK(b.count == 1);  // tracking => no redundant saves
    std::cout << "b.count=" << b.count << '\n' ;
}

int
test_main( int /* argc */, char* /* argv */[] )
{
    A a;
    B b;
    std::string filename;
    filename += boost::archive::tmpdir();
    filename += '/';
    filename += BOOST_PP_STRINGIZE(testfile_);
    filename += BOOST_PP_STRINGIZE(BOOST_ARCHIVE_TEST);
    out(filename.c_str(), a, b);
    return EXIT_SUCCESS;
}

// EOF
