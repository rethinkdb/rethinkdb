/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_iterators.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <vector>
#include <cstdlib> // for rand
#include <functional>
#include <sstream> // used to test stream iterators

#include <boost/config.hpp>
#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{
    using ::rand; 
}
#endif

#include <boost/detail/workaround.hpp>
#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
#include <boost/archive/dinkumware.hpp>
#endif

#ifndef BOOST_NO_CWCHAR
#include <boost/archive/iterators/mb_from_wchar.hpp>
#include <boost/archive/iterators/wchar_from_mb.hpp>
#endif
#include <boost/archive/iterators/xml_escape.hpp>
#include <boost/archive/iterators/xml_unescape.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/istream_iterator.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>

#include "../test/test_tools.hpp"

#ifndef BOOST_NO_CWCHAR

void test_wchar_from_mb(const wchar_t *la, const char * a, const unsigned int size){
    typedef boost::archive::iterators::wchar_from_mb<const char *> translator;
    BOOST_CHECK((
        std::equal(
            translator(BOOST_MAKE_PFTO_WRAPPER(a)),
            translator(BOOST_MAKE_PFTO_WRAPPER(a + size)),
            la
        )
    ));
}

void test_mb_from_wchar(const char * a, const wchar_t *la, const unsigned int size){
    typedef boost::archive::iterators::mb_from_wchar<const wchar_t *> translator;
    BOOST_CHECK(
        std::equal(
            translator(BOOST_MAKE_PFTO_WRAPPER(la)), 
            translator(BOOST_MAKE_PFTO_WRAPPER(la + size)), 
            a
        )
    );
}

#endif

template<class CharType>
void test_xml_escape(
    const CharType * xml_escaped, 
    const CharType * xml, 
    unsigned int size
){
    typedef boost::archive::iterators::xml_escape<const CharType *> translator;

    BOOST_CHECK(
        std::equal(
            translator(BOOST_MAKE_PFTO_WRAPPER(xml)),
            translator(BOOST_MAKE_PFTO_WRAPPER(xml + size)),
            xml_escaped
        )
    );
}

template<class CharType>
void test_xml_unescape(
    const CharType * xml, 
    const CharType * xml_escaped, 
    unsigned int size
){

    // test xml_unescape
    typedef boost::archive::iterators::xml_unescape<const CharType *> translator;

    BOOST_CHECK(
        std::equal(
            translator(BOOST_MAKE_PFTO_WRAPPER(xml_escaped)),
            translator(BOOST_MAKE_PFTO_WRAPPER(xml_escaped + size)),
            xml
        )
    );
}

template<int BitsOut, int BitsIn>
void test_transform_width(unsigned int size){
    // test transform_width
    char rawdata[8];
    
    char * rptr;
    for(rptr = rawdata + 6; rptr-- > rawdata;)
        *rptr = std::rand();

    // convert 8 to 6 bit characters
    typedef boost::archive::iterators::transform_width<
        char *, BitsOut, BitsIn 
    > translator1;

    std::vector<char> v6;

    std::copy(
        translator1(BOOST_MAKE_PFTO_WRAPPER(static_cast<char *>(rawdata))),
        translator1(BOOST_MAKE_PFTO_WRAPPER(rawdata + size)),
        std::back_inserter(v6)
    );

    // check to see we got the expected # of characters out
    if(0 ==  size)
        BOOST_CHECK(v6.size() == 0);
    else
        BOOST_CHECK(v6.size() == (size * BitsIn - 1 ) / BitsOut + 1);

    typedef boost::archive::iterators::transform_width<
        std::vector<char>::iterator, BitsIn, BitsOut
    > translator2;

    BOOST_CHECK(
        std::equal(
            rawdata,
            rawdata + size,
            translator2(BOOST_MAKE_PFTO_WRAPPER(v6.begin()))
        )
    );

}

template<class CharType>
void test_stream_iterators(
    const CharType * test_data, 
    unsigned int size
){
    std::basic_stringstream<CharType> ss;
    boost::archive::iterators::ostream_iterator<CharType> osi =
        boost::archive::iterators::ostream_iterator<CharType>(ss);
    std::copy(test_data, test_data + size, osi);

    BOOST_CHECK(size == ss.str().size());

    boost::archive::iterators::istream_iterator<CharType> isi =
        boost::archive::iterators::istream_iterator<CharType>(ss);
    BOOST_CHECK(std::equal(test_data, test_data + size,isi));
}

int
test_main(int /* argc */, char* /* argv */ [] )
{
    const char xml[] = "<+>+&+\"+'";
    const char xml_escaped[] = "&lt;+&gt;+&amp;+&quot;+&apos;";
    test_xml_escape<const char>(
        xml_escaped, 
        xml, 
        sizeof(xml) / sizeof(char) - 1
    );
    test_xml_unescape<const char>(
        xml, 
        xml_escaped, 
        sizeof(xml_escaped) / sizeof(char) - 1
    );

    const char a[] = "abcdefghijklmnopqrstuvwxyz";

    #ifndef BOOST_NO_CWCHAR
    const wchar_t wxml[] = L"<+>+&+\"+'";
    const wchar_t wxml_escaped[] = L"&lt;+&gt;+&amp;+&quot;+&apos;";
    test_xml_escape<const wchar_t>(
        wxml_escaped, 
        wxml, 
        sizeof(wxml) / sizeof(wchar_t) - 1
    );
    test_xml_unescape<const wchar_t>(
        wxml, 
        wxml_escaped, 
        sizeof(wxml_escaped) / sizeof(wchar_t) - 1
    );

    const wchar_t la[] = L"abcdefghijklmnopqrstuvwxyz";

    test_wchar_from_mb(la, a, sizeof(a) / sizeof(char) - 1);
    test_mb_from_wchar(a, la, sizeof(la) / sizeof(wchar_t) - 1);

    test_stream_iterators<wchar_t>(la, sizeof(la)/sizeof(wchar_t) - 1);
    #endif

    test_stream_iterators<char>(a, sizeof(a) - 1);

    test_transform_width<6, 8>(0);
    test_transform_width<6, 8>(1);
    test_transform_width<6, 8>(2);
    test_transform_width<6, 8>(3);
    test_transform_width<6, 8>(4);
    test_transform_width<6, 8>(5);
    test_transform_width<6, 8>(6);
    test_transform_width<6, 8>(7);
    test_transform_width<6, 8>(8);

    return EXIT_SUCCESS;
}
