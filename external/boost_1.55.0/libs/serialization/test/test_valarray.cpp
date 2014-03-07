/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_valarrray.cpp

// (C) Copyright 2005 Matthias Troyer . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution

#include <cstddef> // NULL
#include <fstream>

#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include "test_tools.hpp"

#include <boost/serialization/valarray.hpp>

int test_main( int /* argc */, char* /* argv */[] )
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    // test array of objects
    std::valarray<int> avalarray(2);
    avalarray[0] = 42;
    avalarray[1] = -42;
    {   
        test_ostream os(testfile, TEST_STREAM_FLAGS);
        test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
        oa << boost::serialization::make_nvp("avalarray", avalarray);
    }
    std::valarray<int> avalarray1;
    {
        test_istream is(testfile, TEST_STREAM_FLAGS);
        test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
        ia >> boost::serialization::make_nvp("avalarray", avalarray1);
    }
    bool equal = (    avalarray.size() == avalarray1.size() 
                   && avalarray[0] == avalarray1[0]
                   && avalarray[1] == avalarray1[1]
                 );
                  
    BOOST_CHECK(equal);
    std::remove(testfile);
    return EXIT_SUCCESS;
}

// EOF
