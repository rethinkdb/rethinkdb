/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_null_ptr.cpp: test implementation level trait

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

#include "test_tools.hpp"
#include <boost/serialization/base_object.hpp>

class polymorphic_base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & /* ar */, const unsigned int /* file_version */){
    }
public:
    virtual ~polymorphic_base(){};
};

class polymorphic_derived1 : public polymorphic_base
{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(polymorphic_base);
    }
};

// save unregistered polymorphic classes
void save(const char *testfile)
{
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);

    polymorphic_base *rb1 =  NULL;
    polymorphic_derived1 *rd1 = NULL;
    
    oa << BOOST_SERIALIZATION_NVP(rb1);
    oa << BOOST_SERIALIZATION_NVP(rd1);
}

// load unregistered polymorphic classes
void load(const char *testfile)
{
    test_istream is(testfile, TEST_STREAM_FLAGS);
    test_iarchive ia(is, TEST_ARCHIVE_FLAGS);

    polymorphic_derived1 dummy;

    polymorphic_base *rb1 = & dummy;
    polymorphic_derived1 *rd1 = & dummy;

    ia >> BOOST_SERIALIZATION_NVP(rb1);
    BOOST_CHECK_MESSAGE(NULL == rb1, "Null pointer not restored");

    ia >> BOOST_SERIALIZATION_NVP(rd1);
    BOOST_CHECK_MESSAGE(NULL == rd1, "Null pointer not restored");

    delete rb1;
    delete rd1;
}

int
test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);
    save(testfile);
    load(testfile);
    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
