/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_complex.cpp

// (C) Copyright 2005 Matthias Troyer . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <fstream>

#include <cstddef> // NULL
#include <cstdlib> // rand
#include <cstdio> // remove
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/limits.hpp>

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::rand; 
    using ::remove;
    #if BOOST_WORKAROUND(BOOST_MSVC, >= 1400) && !defined(UNDER_CE)
        using ::numeric_limits;
    #endif
    #if !defined(__PGIC__)
        using ::fabs;
    #endif
}
#endif

#include <boost/config.hpp>
#include <boost/limits.hpp> 

#include "test_tools.hpp"
#include <boost/preprocessor/stringize.hpp>
#include BOOST_PP_STRINGIZE(BOOST_ARCHIVE_TEST)

#include <boost/serialization/complex.hpp>

int test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    // test array of objects
    std::complex<float> a(
        static_cast<float>(std::rand()),
        static_cast<float>(std::rand())
    );
    std::complex<double> b(
        static_cast<double>(std::rand()),
        static_cast<double>(std::rand())
    );
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os);
        oa << boost::serialization::make_nvp("afloatcomplex", a);
        oa << boost::serialization::make_nvp("adoublecomplex", b);
    }
    std::complex<float> a1;
    std::complex<double> b1;
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is);
        ia >> boost::serialization::make_nvp("afloatcomplex", a1);
        ia >> boost::serialization::make_nvp("adoublecomplex", b1);
    }

    BOOST_CHECK(std::abs(a-a1) <= 2.*std::numeric_limits<float>::round_error());
    BOOST_CHECK(std::abs(b-b1) <= 2.*std::numeric_limits<double>::round_error());

    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
