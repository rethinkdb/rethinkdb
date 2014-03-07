/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_dll_simple.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

// invoke header for a custom archive test.

#include <cstddef> // NULL
#include <fstream>

#include <cstdio> // remove
#include <boost/config.hpp>

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

// for now, only test with simple text and polymorphic archive
#include "test_tools.hpp"
#include "text_archive.hpp"

#include <boost/archive/polymorphic_text_oarchive.hpp>
#include <boost/archive/polymorphic_text_iarchive.hpp>

#define A_IMPORT
#include "A.hpp"

// simple class with text archive compiled in dll
void
test1(){
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    const A a;
    A a1;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        boost::archive::text_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("a", a);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        boost::archive::text_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("a", a1);
    }
    BOOST_CHECK_EQUAL(a, a1);

    std::remove(testfile);
}

// simple class with polymorphic archive compiled in dll
void
test2(){
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    const A a;
    A a1;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        boost::archive::polymorphic_text_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        boost::archive::polymorphic_oarchive & poa(oa);
        poa << boost::serialization::make_nvp("a", a);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        boost::archive::polymorphic_text_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        boost::archive::polymorphic_iarchive & pia(ia);
        pia >> boost::serialization::make_nvp("a", a1);
    }
    BOOST_CHECK_EQUAL(a, a1);

    std::remove(testfile);
}

// simple class pointer with text archive compiled in dll
void
test3(){
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    const A *a = new A;
    A *a1;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        boost::archive::text_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("a", a);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        boost::archive::text_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("a", a1);
    }
    BOOST_CHECK_EQUAL(*a, *a1);

    std::remove(testfile);
    delete a;
}

// simple class pointer with polymorphic archive compiled in dll
void
test4(){
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    const A *a = new A;
    A *a1;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        boost::archive::polymorphic_text_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        boost::archive::polymorphic_oarchive & poa(oa);
        poa << boost::serialization::make_nvp("a", a);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        boost::archive::polymorphic_text_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        boost::archive::polymorphic_iarchive & pia(ia);
        pia >> boost::serialization::make_nvp("a", a1);
    }
    BOOST_CHECK_EQUAL(*a, *a1);

    std::remove(testfile);
    delete a;
}

#include "B.hpp"

// derived class with base text archive compiled in dll
void
test5(){
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    const B b;
    B b1;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        boost::archive::text_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("b", b);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        boost::archive::text_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("b", b1);
    }
    BOOST_CHECK_EQUAL(b, b1);

    std::remove(testfile);
}

// derived class with base base compiled with polymorphic archive in dll
void
test6(){
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    const B b;
    B b1;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        boost::archive::polymorphic_text_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        boost::archive::polymorphic_oarchive & poa(oa);
        poa << boost::serialization::make_nvp("b", b);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        boost::archive::polymorphic_text_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        boost::archive::polymorphic_iarchive & pia(ia);
        pia >> boost::serialization::make_nvp("b", b1);
    }
    BOOST_CHECK_EQUAL(b, b1);

    std::remove(testfile);
}

// derived class pointer with base text archive compiled in dll
void
test7(){
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    const B *b = new B;
    B *b1;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        boost::archive::text_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("b", b);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        boost::archive::text_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("b", b1);
    }
    BOOST_CHECK_EQUAL(*b, *b1);

    std::remove(testfile);
    delete b;
}

// derived class pointer with base polymorphic archive compiled in dll
void
test8(){
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    const B *b = new B;
    B *b1;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        boost::archive::polymorphic_text_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        boost::archive::polymorphic_oarchive & poa(oa);
        poa << boost::serialization::make_nvp("b", b);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        boost::archive::polymorphic_text_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        boost::archive::polymorphic_iarchive & pia(ia);
        pia >> boost::serialization::make_nvp("b", b1);
    }
    BOOST_CHECK_EQUAL(*b, *b1);

    std::remove(testfile);
    delete b;
}


int 
test_main( int /* argc */, char* /* argv */[] )
{
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    return EXIT_SUCCESS;
}

