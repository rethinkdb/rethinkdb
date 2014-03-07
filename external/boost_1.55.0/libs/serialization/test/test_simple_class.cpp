/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_simple_class.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

// invoke header for a custom archive test.

#include <cstddef> // NULL
#include <cstdio> // remove
#include <fstream>
#include <cmath>

#include <boost/config.hpp>

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include "test_tools.hpp"

#include "A.hpp"
#include "A.ipp"

bool A::check_equal(const A &rhs) const
{
    BOOST_CHECK_EQUAL(b, rhs.b);
    BOOST_CHECK_EQUAL(l, rhs.l);
    #ifndef BOOST_NO_INT64_T
    BOOST_CHECK_EQUAL(f, rhs.f);
    BOOST_CHECK_EQUAL(g, rhs.g);
    #endif
    BOOST_CHECK_EQUAL(m, rhs.m);
    BOOST_CHECK_EQUAL(n, rhs.n);
    BOOST_CHECK_EQUAL(o, rhs.o);
    BOOST_CHECK_EQUAL(p, rhs.p);
    BOOST_CHECK_EQUAL(q, rhs.q);
    #ifndef BOOST_NO_CWCHAR
    BOOST_CHECK_EQUAL(r, rhs.r);
    #endif
    BOOST_CHECK_EQUAL(c, rhs.c);
    BOOST_CHECK_EQUAL(s, rhs.s);
    BOOST_CHECK_EQUAL(t, rhs.t);
    BOOST_CHECK_EQUAL(u, rhs.u);
    BOOST_CHECK_EQUAL(v, rhs.v);
    BOOST_CHECK_EQUAL(l, rhs.l);
    BOOST_CHECK(!(
        w == 0 
        && std::fabs(rhs.w) > std::numeric_limits<float>::epsilon()
    ));
    BOOST_CHECK(!(
        std::fabs(rhs.w/w - 1.0) > std::numeric_limits<float>::epsilon()
    ));
    BOOST_CHECK(!(
        x == 0 && std::fabs(rhs.x - x) > std::numeric_limits<float>::epsilon()
    ));
    BOOST_CHECK(!(
        std::fabs(rhs.x/x - 1.0) > std::numeric_limits<float>::epsilon()
    ));
    BOOST_CHECK(!(0 != y.compare(rhs.y)));
    #ifndef BOOST_NO_STD_WSTRING
    BOOST_CHECK(!(0 != z.compare(rhs.z)));
    #endif      
    return true;
}

int 
test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    A a, a1;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("a", a);
    }
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("a", a1);
    }
    a.check_equal(a1);
    std::remove(testfile);
    return 0;
}
