//  Unit test for boost::lexical_cast.
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2011-2012.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).


#include <boost/config.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>

#include <iostream>


///////////////////////// char streamable classes ///////////////////////////////////////////

struct streamable_easy { enum ENU {value = 0}; };
std::ostream& operator << (std::ostream& ostr, const streamable_easy&) {
    return ostr << streamable_easy::value;
}
std::istream& operator >> (std::istream& istr, const streamable_easy&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, streamable_easy::value);
    return istr;
}

struct streamable_medium { enum ENU {value = 1}; };
template <class CharT>
typename boost::enable_if<boost::is_same<CharT, char>, std::basic_ostream<CharT>&>::type 
operator << (std::basic_ostream<CharT>& ostr, const streamable_medium&) {
    return ostr << streamable_medium::value;
}
template <class CharT>
typename boost::enable_if<boost::is_same<CharT, char>, std::basic_istream<CharT>&>::type 
operator >> (std::basic_istream<CharT>& istr, const streamable_medium&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, streamable_medium::value);
    return istr;
}

struct streamable_hard { enum ENU {value = 2}; };
template <class CharT, class TraitsT>
typename boost::enable_if<boost::is_same<CharT, char>, std::basic_ostream<CharT, TraitsT>&>::type 
operator << (std::basic_ostream<CharT, TraitsT>& ostr, const streamable_hard&) {
    return ostr << streamable_hard::value;
}
template <class CharT, class TraitsT>
typename boost::enable_if<boost::is_same<CharT, char>, std::basic_istream<CharT, TraitsT>&>::type 
operator >> (std::basic_istream<CharT, TraitsT>& istr, const streamable_hard&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, streamable_hard::value);
    return istr;
}

struct streamable_hard2 { enum ENU {value = 3}; };
template <class TraitsT>
std::basic_ostream<char, TraitsT>& operator << (std::basic_ostream<char, TraitsT>& ostr, const streamable_hard2&) {
    return ostr << streamable_hard2::value;
}
template <class TraitsT>
std::basic_istream<char, TraitsT>& operator >> (std::basic_istream<char, TraitsT>& istr, const streamable_hard2&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, streamable_hard2::value);
    return istr;
}


///////////////////////// wchar_t streamable classes ///////////////////////////////////////////

struct wstreamable_easy { enum ENU {value = 4}; };
std::wostream& operator << (std::wostream& ostr, const wstreamable_easy&) {
    return ostr << wstreamable_easy::value;
}
std::wistream& operator >> (std::wistream& istr, const wstreamable_easy&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, wstreamable_easy::value);
    return istr;
}

struct wstreamable_medium { enum ENU {value = 5}; };
template <class CharT>
typename boost::enable_if<boost::is_same<CharT, wchar_t>, std::basic_ostream<CharT>& >::type 
operator << (std::basic_ostream<CharT>& ostr, const wstreamable_medium&) {
    return ostr << wstreamable_medium::value;
}
template <class CharT>
typename boost::enable_if<boost::is_same<CharT, wchar_t>, std::basic_istream<CharT>& >::type 
operator >> (std::basic_istream<CharT>& istr, const wstreamable_medium&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, wstreamable_medium::value);
    return istr;
}

struct wstreamable_hard { enum ENU {value = 6}; };
template <class CharT, class TraitsT>
typename boost::enable_if<boost::is_same<CharT, wchar_t>, std::basic_ostream<CharT, TraitsT>&>::type 
operator << (std::basic_ostream<CharT, TraitsT>& ostr, const wstreamable_hard&) {
    return ostr << wstreamable_hard::value;
}
template <class CharT, class TraitsT>
typename boost::enable_if<boost::is_same<CharT, wchar_t>, std::basic_istream<CharT, TraitsT>&>::type 
operator >> (std::basic_istream<CharT, TraitsT>& istr, const wstreamable_hard&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, wstreamable_hard::value);
    return istr;
}

struct wstreamable_hard2 { enum ENU {value = 7}; };
template <class TraitsT>
std::basic_ostream<wchar_t, TraitsT>& operator << (std::basic_ostream<wchar_t, TraitsT>& ostr, const wstreamable_hard2&) {
    return ostr << wstreamable_hard2::value;
}
template <class TraitsT>
std::basic_istream<wchar_t, TraitsT>& operator >> (std::basic_istream<wchar_t, TraitsT>& istr, const wstreamable_hard2&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, wstreamable_hard2::value);
    return istr;
}

///////////////////////// char and wchar_t streamable classes ///////////////////////////////////////////


struct bistreamable_easy { enum ENU {value = 8}; };
std::ostream& operator << (std::ostream& ostr, const bistreamable_easy&) {
    return ostr << bistreamable_easy::value;
}
std::istream& operator >> (std::istream& istr, const bistreamable_easy&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, bistreamable_easy::value);
    return istr;
}

std::wostream& operator << (std::wostream& ostr, const bistreamable_easy&) {
    return ostr << bistreamable_easy::value + 100;
}
std::wistream& operator >> (std::wistream& istr, const bistreamable_easy&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, bistreamable_easy::value + 100);
    return istr;
}

struct bistreamable_medium { enum ENU {value = 9}; };
template <class CharT>
std::basic_ostream<CharT>& operator << (std::basic_ostream<CharT>& ostr, const bistreamable_medium&) {
    return ostr << bistreamable_medium::value + (sizeof(CharT) == 1 ? 0 : 100);
}
template <class CharT>
std::basic_istream<CharT>& operator >> (std::basic_istream<CharT>& istr, const bistreamable_medium&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, bistreamable_medium::value + (sizeof(CharT) == 1 ? 0 : 100));
    return istr;
}

struct bistreamable_hard { enum ENU {value = 10}; };
template <class CharT, class TraitsT>
std::basic_ostream<CharT, TraitsT>& operator << (std::basic_ostream<CharT, TraitsT>& ostr, const bistreamable_hard&) {
    return ostr << bistreamable_hard::value + (sizeof(CharT) == 1 ? 0 : 100);
}
template <class CharT, class TraitsT>
std::basic_istream<CharT, TraitsT>& operator >> (std::basic_istream<CharT, TraitsT>& istr, const bistreamable_hard&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, bistreamable_hard::value + (sizeof(CharT) == 1 ? 0 : 100));
    return istr;
}

struct bistreamable_hard2 { enum ENU {value = 11}; };
template <class TraitsT>
std::basic_ostream<char, TraitsT>& operator << (std::basic_ostream<char, TraitsT>& ostr, const bistreamable_hard2&) {
    return ostr << bistreamable_hard2::value;
}
template <class TraitsT>
std::basic_istream<char, TraitsT>& operator >> (std::basic_istream<char, TraitsT>& istr, const bistreamable_hard2&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, bistreamable_hard2::value);
    return istr;
}

template <class TraitsT>
std::basic_ostream<wchar_t, TraitsT>& operator << (std::basic_ostream<wchar_t, TraitsT>& ostr, const bistreamable_hard2&) {
    return ostr << bistreamable_hard2::value + 100;
}
template <class TraitsT>
std::basic_istream<wchar_t, TraitsT>& operator >> (std::basic_istream<wchar_t, TraitsT>& istr, const bistreamable_hard2&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, bistreamable_hard2::value + 100);
    return istr;
}


void test_ostream_character_detection();
void test_istream_character_detection();
void test_mixed_stream_character_detection();

boost::unit_test::test_suite *init_unit_test_suite(int, char *[])
{
    boost::unit_test::test_suite *suite = 
        BOOST_TEST_SUITE("lexical_cast stream character detection");
    suite->add(BOOST_TEST_CASE(&test_ostream_character_detection));
    suite->add(BOOST_TEST_CASE(&test_istream_character_detection));
    suite->add(BOOST_TEST_CASE(&test_mixed_stream_character_detection));
    
    return suite;
}

template <class T>
static void test_ostr_impl() {
    T streamable;
    BOOST_CHECK_EQUAL(T::value,  boost::lexical_cast<int>(streamable));
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(T::value),  boost::lexical_cast<std::string>(streamable));
}

template <class T>
static void test_wostr_impl() {
    T streamable;
    BOOST_CHECK_EQUAL(T::value,  boost::lexical_cast<int>(streamable));
    // BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(T::value), boost::lexical_cast<std::string>(streamable)); // Shall not compile???
    BOOST_CHECK(boost::lexical_cast<std::wstring>(T::value) == boost::lexical_cast<std::wstring>(streamable));
}

template <class T>
static void test_bistr_impl() {
    T streamable;

    BOOST_CHECK_EQUAL(T::value,  boost::lexical_cast<int>(streamable));
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(T::value),  boost::lexical_cast<std::string>(streamable));

    BOOST_CHECK(boost::lexical_cast<std::wstring>(T::value + 100) == boost::lexical_cast<std::wstring>(streamable));
}

void test_ostream_character_detection() {
    test_ostr_impl<streamable_easy>();
    test_ostr_impl<streamable_medium>();
    test_ostr_impl<streamable_hard>();
    test_ostr_impl<streamable_hard2>();
    
    test_wostr_impl<wstreamable_easy>();
    test_wostr_impl<wstreamable_medium>();
    test_wostr_impl<wstreamable_hard>();
    test_wostr_impl<wstreamable_hard2>();
    
    test_bistr_impl<bistreamable_easy>();
    test_bistr_impl<bistreamable_medium>();
    test_bistr_impl<bistreamable_hard>();
    test_bistr_impl<bistreamable_hard2>();
}


template <class T>
static void test_istr_impl() {
    boost::lexical_cast<T>(T::value);
    boost::lexical_cast<T>(boost::lexical_cast<std::string>(T::value));
}

template <class T>
static void test_wistr_impl() {
    boost::lexical_cast<T>(T::value);
    //boost::lexical_cast<T>(boost::lexical_cast<std::string>(T::value)); // Shall not compile???
    boost::lexical_cast<T>(boost::lexical_cast<std::wstring>(T::value));
}

template <class T>
static void test_bistr_instr_impl() {
    boost::lexical_cast<T>(T::value);
    boost::lexical_cast<T>(boost::lexical_cast<std::string>(T::value));
    boost::lexical_cast<T>(boost::lexical_cast<std::wstring>(T::value + 100));
}

void test_istream_character_detection() {
    test_istr_impl<streamable_easy>();
    test_istr_impl<streamable_medium>();
    test_istr_impl<streamable_hard>();
    test_istr_impl<streamable_hard2>();
    
    test_wistr_impl<wstreamable_easy>();
    test_wistr_impl<wstreamable_medium>();
    test_wistr_impl<wstreamable_hard>();
    test_wistr_impl<wstreamable_hard2>();
    
    test_bistr_instr_impl<bistreamable_easy>();
    test_bistr_instr_impl<bistreamable_medium>();
    test_bistr_instr_impl<bistreamable_hard>();
    test_bistr_instr_impl<bistreamable_hard2>();
}






struct wistreamble_ostreamable { enum ENU {value = 200}; };
std::ostream& operator << (std::ostream& ostr, const wistreamble_ostreamable&) {
    return ostr << wistreamble_ostreamable::value;
}
std::wistream& operator >> (std::wistream& istr, const wistreamble_ostreamable&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, wistreamble_ostreamable::value);
    return istr;
}

struct istreamble_wostreamable { enum ENU {value = 201}; };
std::wostream& operator << (std::wostream& ostr, const istreamble_wostreamable&) {
    return ostr << istreamble_wostreamable::value;
}
std::istream& operator >> (std::istream& istr, const istreamble_wostreamable&) {
    int i; istr >> i; BOOST_CHECK_EQUAL(i, istreamble_wostreamable::value);
    return istr;
}

void test_mixed_stream_character_detection() {
    //boost::lexical_cast<std::wstring>(std::string("qwe")); // TODO: ALLOW IT AS EXTENSION!
    
    boost::lexical_cast<wistreamble_ostreamable>(wistreamble_ostreamable::value);
    BOOST_CHECK_EQUAL(boost::lexical_cast<int>(wistreamble_ostreamable()), wistreamble_ostreamable::value);
    
    boost::lexical_cast<istreamble_wostreamable>(istreamble_wostreamable::value);
    BOOST_CHECK_EQUAL(boost::lexical_cast<int>(istreamble_wostreamable()), istreamble_wostreamable::value);
}
