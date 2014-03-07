/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_codecvt_null.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution.  Note: compilation with compilers
// which use wchar_t as 2 byte objects will emit warnings.  These should be
// ignored.

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <locale>
#include <vector>
#include <cstddef>

#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include "test_tools.hpp"

#include <boost/archive/add_facet.hpp>
#include <boost/archive/codecvt_null.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/archive/iterators/istream_iterator.hpp>

template<std::size_t S>
struct test_data
{
    static wchar_t wchar_encoding[];
};

template<>
wchar_t test_data<2>::wchar_encoding[] = {
    0x0001,
    0x007f,
    0x0080,
    0x07ff,
    0x0800,
    0x7fff
};

template<>
wchar_t test_data<4>::wchar_encoding[] = {
    0x00000001,
    0x0000007f,
    0x00000080,
    0x000007ff,
    0x00000800,
    0x0000ffff,
    0x00010000,
    0x0010ffff,
    0x001fffff,
    0x00200000,
    0x03ffffff,
    0x04000000,
    0x7fffffff
};

#include <iostream>

int test_main( int /* argc */, char* /* argv */[] ) {
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    std::locale old_loc;
    std::locale * null_locale = 
        boost::archive::add_facet(old_loc, new boost::archive::codecvt_null<wchar_t>);

    typedef test_data<sizeof(wchar_t)> td;
    {
        std::wofstream ofs;
        ofs.imbue(*null_locale);
        ofs.open(testfile, std::ios::binary);
        std::copy(
            td::wchar_encoding,
            #if ! defined(__BORLANDC__)
                // borland 5.60 complains about this
                td::wchar_encoding + sizeof(td::wchar_encoding)/sizeof(wchar_t),
            #else
                // so use this instead
                td::wchar_encoding + 6,
            #endif
            boost::archive::iterators::ostream_iterator<wchar_t>(ofs)
        );
    }
    bool ok = false;
    {
        std::wifstream ifs;
        ifs.imbue(*null_locale);
        ifs.open(testfile, std::ios::binary);
        ok = std::equal(
            td::wchar_encoding,
            #if ! defined(__BORLANDC__)
                // borland 5.60 complains about this
                td::wchar_encoding + sizeof(td::wchar_encoding)/sizeof(wchar_t),
            #else
                // so use this instead
                td::wchar_encoding + 6,
            #endif
            boost::archive::iterators::istream_iterator<wchar_t>(ifs)
        );
    }

    BOOST_CHECK(ok);
    {
        std::wofstream ofs("testfile2");
        ofs.imbue(*null_locale);
        int i = 10;
        ofs << i;
        ofs.close();
        
        std::wifstream ifs("testfile2");
        ifs.imbue(*null_locale);
        int i2;
        ifs >> i2;
        std::cout << "i=" << i << std::endl;
        std::cout << "i2=" << i2 << std::endl;
        BOOST_CHECK(i == i2);
        ifs.close();
    }
 
    delete null_locale;
    std::remove(testfile);
    return EXIT_SUCCESS;
}

