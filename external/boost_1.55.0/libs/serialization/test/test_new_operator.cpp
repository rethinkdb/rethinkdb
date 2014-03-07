/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_new_operator.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <cstddef> // NULL
#include <cstdio> // remove
#include <fstream>
#include <new>

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include <boost/serialization/access.hpp>

#include "test_tools.hpp"

#include "A.hpp"
#include "A.ipp"

class ANew : public A {
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned /*file_version*/){
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(A);
    }
public:
    static unsigned int m_new_calls;
    static unsigned int m_delete_calls;
    // implement class specific new/delete in terms standard
    // implementation - we're testing serialization 
    // not "new" here.
    static void * operator new(size_t s){
        ++m_new_calls;
        return  ::operator new(s);
    }
    static void operator delete(void *p, std::size_t /*s*/){
        ++m_delete_calls;
        ::operator delete(p);
    }
};

unsigned int ANew::m_new_calls = 0;
unsigned int ANew::m_delete_calls = 0;

int test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    
    BOOST_REQUIRE(NULL != testfile);

    ANew *ta = new ANew();

    BOOST_CHECK(1 == ANew::m_new_calls);
    BOOST_CHECK(0 == ANew::m_delete_calls);

    ANew *ta1 = NULL;

    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("ta", ta);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("ta", ta1);
    }
    BOOST_CHECK(ta != ta1);
    BOOST_CHECK(*ta == *ta1);

    BOOST_CHECK(2 == ANew::m_new_calls);
    BOOST_CHECK(0 == ANew::m_delete_calls);

    std::remove(testfile);

    delete ta;
    delete ta1;

    BOOST_CHECK(2 == ANew::m_new_calls);
    BOOST_CHECK(2 == ANew::m_delete_calls);

    return EXIT_SUCCESS;
}

// EOF
