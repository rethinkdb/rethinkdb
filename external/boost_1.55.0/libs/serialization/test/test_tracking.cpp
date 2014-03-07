/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_tracking_save.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <cstddef> // NULL
#include <fstream>

#include <boost/config.hpp>
#include <cstdio> // remove
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include "test_tools.hpp"
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/nvp.hpp>

#define TEST_CLASS(N, TRACKING) \
class N \
{ \
    friend class boost::serialization::access; \
    template<class Archive> \
    void serialize(Archive & /* ar */, const unsigned int /* file_version */){ \
        ++count; \
    } \
public: \
    static unsigned int count; \
}; \
unsigned int N::count = 0;\
BOOST_CLASS_TRACKING(N, TRACKING)

TEST_CLASS(AN, ::boost::serialization::track_never)
TEST_CLASS(AS, ::boost::serialization::track_selectively)
TEST_CLASS(AA, ::boost::serialization::track_always)

// test pointers
TEST_CLASS(PAN, ::boost::serialization::track_never)
TEST_CLASS(PAS, ::boost::serialization::track_selectively)
TEST_CLASS(PAA, ::boost::serialization::track_always)

void out(const char *testfile)
{
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
    // write object twice to check tracking
    AN an;
    AS as;
    AA aa;
    oa << BOOST_SERIALIZATION_NVP(an) << BOOST_SERIALIZATION_NVP(an);
    BOOST_CHECK(an.count == 2);
    oa << BOOST_SERIALIZATION_NVP(as) << BOOST_SERIALIZATION_NVP(as);
    BOOST_CHECK(as.count == 2);
    oa << BOOST_SERIALIZATION_NVP(aa) << BOOST_SERIALIZATION_NVP(aa);
    BOOST_CHECK(aa.count == 1);

    PAN *pan = new PAN;
    PAS *pas = new PAS;
    PAA *paa = new PAA;
    oa << BOOST_SERIALIZATION_NVP(pan) << BOOST_SERIALIZATION_NVP(pan);
    BOOST_CHECK(pan->count == 2);
    oa << BOOST_SERIALIZATION_NVP(pas) << BOOST_SERIALIZATION_NVP(pas);
    BOOST_CHECK(pas->count == 1);
    oa << BOOST_SERIALIZATION_NVP(paa) << BOOST_SERIALIZATION_NVP(paa);
    BOOST_CHECK(paa->count == 1);
    delete pan;
    delete pas;
    delete paa;
}

void in(const char *testfile)
{
    test_istream is(testfile, TEST_STREAM_FLAGS);
    test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
    // read object twice to check tracking
    AN an;
    AS as;
    AA aa;

    AN::count = 0;
    AS::count = 0;
    AA::count = 0;

    ia >> BOOST_SERIALIZATION_NVP(an) >> BOOST_SERIALIZATION_NVP(an);
    BOOST_CHECK(an.count == 2);
    ia >> BOOST_SERIALIZATION_NVP(as) >> BOOST_SERIALIZATION_NVP(as);
    BOOST_CHECK(as.count == 2);
    ia >> BOOST_SERIALIZATION_NVP(aa) >> BOOST_SERIALIZATION_NVP(aa);
    BOOST_CHECK(aa.count == 1);

    PAN::count = 0;
    PAS::count = 0;
    PAA::count = 0;

    PAN *pan = NULL;
    PAS *pas = NULL;
    PAA *paa = NULL;
    ia >> BOOST_SERIALIZATION_NVP(pan);
    ia >> BOOST_SERIALIZATION_NVP(pan);
    BOOST_CHECK(pan->count == 2);
    ia >> BOOST_SERIALIZATION_NVP(pas);
    ia >> BOOST_SERIALIZATION_NVP(pas);
    BOOST_CHECK(pas->count == 1);
    ia >> BOOST_SERIALIZATION_NVP(paa);
    ia >> BOOST_SERIALIZATION_NVP(paa);
    BOOST_CHECK(paa->count == 1);
    delete pan;
    delete pas;
    delete paa;
}

int
test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    out(testfile);
    in(testfile);
    std::remove(testfile);
    return EXIT_SUCCESS;
}

