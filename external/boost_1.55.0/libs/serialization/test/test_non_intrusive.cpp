/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_non_intrursive.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

// this tests:
// a) non-intrusive method of implementing serialization
// b) usage of a non-default constructor

#include <fstream>
#include <cstdlib> // for rand()
#include <cstdio>  // remove
#include <cmath>   // for fabs()
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/limits.hpp>

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::rand; 
    using ::fabs; 
    using ::remove;
    #if BOOST_WORKAROUND(BOOST_MSVC, >= 1400) && !defined(UNDER_CE)
        using ::numeric_limits;
    #endif
}
#endif

#include <boost/archive/archive_exception.hpp>
#include "test_tools.hpp"

///////////////////////////////////////////////////////
// simple class test - using non-intrusive syntax
// illustrates the usage of the non-intrusve syntax
class A
{
public:
    signed char s;
    unsigned char t;
    signed int u;
    unsigned int v;
    float w;
    double x;
    A();
    bool operator==(const A & rhs) const;
    bool operator<(const A & rhs) const;
};

A::A() : 
    s(static_cast<signed char>(0xff & std::rand())),
    t(static_cast<signed char>(0xff & std::rand())),
    u(std::rand()),
    v(std::rand()),
    w((float)std::rand() / std::rand()),
    x((double)std::rand() / std::rand())
{
}

bool A::operator==(const A &rhs) const
{
    return
        s == rhs.s 
        && t == rhs.t 
        && u == rhs.u 
        && v == rhs.v 
        && std::fabs(w - rhs.w) <= std::numeric_limits<float>::round_error()
        && std::fabs(x - rhs.x) <= std::numeric_limits<float>::round_error()
    ;
}

bool A::operator<(const A &rhs) const
{
    if(! (s == rhs.s) )
        return s < rhs.s;
    if(! (t == rhs.t) )
        return t < rhs.t;
    if(! (u == rhs.u) )
        return t < rhs.u; 
    if(! (v == rhs.v) )
        return t < rhs.v;
    if(! (std::fabs(w - rhs.w) < std::numeric_limits<float>::round_error() ) )
        return t < rhs.w; 
    if(! (std::fabs(x - rhs.x) < std::numeric_limits<float>::round_error() ) )
        return t < rhs.x;
    return false;
}

// note the following:

// function specializations must be defined in the appropriate
// namespace - boost::serialization
namespace boost { 
namespace serialization {

// This first set of overrides should work with all compilers.

// The last argument is int while the default versions
// defined in serialization.hpp have long as the last argument.
// This is part of the work around for compilers that don't 
// support correct function template ordering.  These functions
// are always called with 0 (i.e. an int) as the last argument.
// Our specialized versions also have int as the last argument
// while the default versions have a long as the last argument.
// This makes our specialized versions a better match than the
// default ones as no argument conversion is required to make a match
template<class Archive>
void serialize(
    Archive & ar, 
    A & a, 
    const unsigned int /* file_version */
){
    ar & boost::serialization::make_nvp("s", a.s);
    ar & boost::serialization::make_nvp("t", a.t);
    ar & boost::serialization::make_nvp("u", a.u);
    ar & boost::serialization::make_nvp("v", a.v);
    ar & boost::serialization::make_nvp("w", a.w);
    ar & boost::serialization::make_nvp("x", a.x);
}

} // serialization
} // namespace boost

void save(const char * testfile){
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
    A a;

    oa << BOOST_SERIALIZATION_NVP(a);
    
    // save a copy pointer to this item
    A *pa1 = &a;
    oa << BOOST_SERIALIZATION_NVP(pa1);

    // save pointer to a new object
    A *pa2 = new A();
    oa << BOOST_SERIALIZATION_NVP(pa2);

    delete pa2;
}

void load(const char * testfile){
    test_istream is(testfile, TEST_STREAM_FLAGS);
    test_iarchive ia(is, TEST_ARCHIVE_FLAGS);

    A a;
    ia >> BOOST_SERIALIZATION_NVP(a);

    A *pa1;
    ia >> BOOST_SERIALIZATION_NVP(pa1);
    BOOST_CHECK_MESSAGE(pa1 == &a, "Copy of pointer not correctly restored");

    A *pa2;
    ia >> BOOST_SERIALIZATION_NVP(pa2);
    BOOST_CHECK_MESSAGE(pa2 != &a, "Pointer not correctly restored");

    delete pa2;
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
