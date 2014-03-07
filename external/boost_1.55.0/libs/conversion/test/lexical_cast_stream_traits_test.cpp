//  Unit test for boost::lexical_cast.
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2012.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).

#include <boost/config.hpp>

#include <boost/lexical_cast.hpp>

#include <boost/test/unit_test.hpp>

template <class T>
static void test_optimized_types_to_string_const()
{
    namespace de = boost::detail;
    typedef de::lexical_cast_stream_traits<T, std::string> trait_1;
    BOOST_CHECK(!trait_1::is_source_input_not_optimized_t::value);
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_1::src_char_t, char>::value));
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_1::target_char_t, char>::value));
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_1::char_type, char>::value));
    BOOST_CHECK(!trait_1::is_string_widening_required_t::value);
    BOOST_CHECK(!trait_1::is_source_input_not_optimized_t::value);
        
    typedef de::lexical_cast_stream_traits<const T, std::string> trait_2;
    BOOST_CHECK(!trait_2::is_source_input_not_optimized_t::value);
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_2::src_char_t, char>::value));
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_2::target_char_t, char>::value));
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_2::char_type, char>::value));
    BOOST_CHECK(!trait_2::is_string_widening_required_t::value);
    BOOST_CHECK(!trait_2::is_source_input_not_optimized_t::value);

    typedef de::lexical_cast_stream_traits<T, std::wstring> trait_3;
    BOOST_CHECK(!trait_3::is_source_input_not_optimized_t::value);
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_3::src_char_t, char>::value));
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_3::target_char_t, wchar_t>::value));
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_3::char_type, wchar_t>::value));

    BOOST_CHECK((boost::detail::is_char_or_wchar<BOOST_DEDUCED_TYPENAME trait_3::no_cv_src>::value != trait_3::is_string_widening_required_t::value));

    BOOST_CHECK(!trait_3::is_source_input_not_optimized_t::value);
}    


template <class T>
static void test_optimized_types_to_string()
{
    test_optimized_types_to_string_const<T>();

    namespace de = boost::detail;
    typedef de::lexical_cast_stream_traits<std::string, T> trait_4;
    BOOST_CHECK(!trait_4::is_source_input_not_optimized_t::value);
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_4::src_char_t, char>::value));
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_4::target_char_t, char>::value));
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_4::char_type, char>::value));
    BOOST_CHECK(!trait_4::is_string_widening_required_t::value);
    BOOST_CHECK(!trait_4::is_source_input_not_optimized_t::value);
        
    typedef de::lexical_cast_stream_traits<const std::string, T> trait_5;
    BOOST_CHECK(!trait_5::is_source_input_not_optimized_t::value);
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_5::src_char_t, char>::value));
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_5::target_char_t, char>::value));
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_5::char_type, char>::value));
    BOOST_CHECK(!trait_5::is_string_widening_required_t::value);
    BOOST_CHECK(!trait_5::is_source_input_not_optimized_t::value);

    typedef de::lexical_cast_stream_traits<const std::wstring, T> trait_6;
    BOOST_CHECK(!trait_6::is_source_input_not_optimized_t::value);
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_6::src_char_t, wchar_t>::value));
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_6::target_char_t, char>::value));
    BOOST_CHECK((boost::is_same<BOOST_DEDUCED_TYPENAME trait_6::char_type, wchar_t>::value));
    BOOST_CHECK(!trait_6::is_string_widening_required_t::value);
}

void test_metafunctions()
{
    test_optimized_types_to_string<bool>();
    test_optimized_types_to_string<char>();
    test_optimized_types_to_string<unsigned char>();
    test_optimized_types_to_string<signed char>();
    test_optimized_types_to_string<short>();
    test_optimized_types_to_string<unsigned short>();
    test_optimized_types_to_string<int>();
    test_optimized_types_to_string<unsigned int>();
    test_optimized_types_to_string<long>();
    test_optimized_types_to_string<unsigned long>();

#if defined(BOOST_HAS_LONG_LONG)
    test_optimized_types_to_string<boost::ulong_long_type>();
    test_optimized_types_to_string<boost::long_long_type>();
#elif defined(BOOST_HAS_MS_INT64)
    test_optimized_types_to_string<unsigned __int64>();
    test_optimized_types_to_string<__int64>();
#endif

#if !defined(BOOST_NO_SWPRINTF) && !defined(__MINGW32__)
    test_optimized_types_to_string<float>();
#endif
    
    test_optimized_types_to_string<std::string>();
    test_optimized_types_to_string<char*>();
    //test_optimized_types_to_string<char[5]>();
    //test_optimized_types_to_string<char[1]>();
    test_optimized_types_to_string<unsigned char*>();
    //test_optimized_types_to_string<unsigned char[5]>();
    //test_optimized_types_to_string<unsigned char[1]>();
    test_optimized_types_to_string<signed char*>();
    //test_optimized_types_to_string<signed char[5]>();
    //test_optimized_types_to_string<signed char[1]>();
    test_optimized_types_to_string<boost::array<char, 1> >();
    test_optimized_types_to_string<boost::array<char, 5> >();
    test_optimized_types_to_string<boost::array<unsigned char, 1> >();
    test_optimized_types_to_string<boost::array<unsigned char, 5> >();
    test_optimized_types_to_string<boost::array<signed char, 1> >();
    test_optimized_types_to_string<boost::array<signed char, 5> >();
    test_optimized_types_to_string<boost::iterator_range<char*> >();
    test_optimized_types_to_string<boost::iterator_range<unsigned char*> >();
    test_optimized_types_to_string<boost::iterator_range<signed char*> >();

    test_optimized_types_to_string_const<boost::array<const char, 1> >();
    test_optimized_types_to_string_const<boost::array<const char, 5> >();
    test_optimized_types_to_string_const<boost::array<const unsigned char, 1> >();
    test_optimized_types_to_string_const<boost::array<const unsigned char, 5> >();
    test_optimized_types_to_string_const<boost::array<const signed char, 1> >();
    test_optimized_types_to_string_const<boost::array<const signed char, 5> >();
    test_optimized_types_to_string_const<boost::iterator_range<const char*> >();
    test_optimized_types_to_string_const<boost::iterator_range<const unsigned char*> >();
    test_optimized_types_to_string_const<boost::iterator_range<const signed char*> >();

#ifndef BOOST_NO_CXX11_HDR_ARRAY
    test_optimized_types_to_string<std::array<char, 1> >();
    test_optimized_types_to_string<std::array<char, 5> >();
    test_optimized_types_to_string<std::array<unsigned char, 1> >();
    test_optimized_types_to_string<std::array<unsigned char, 5> >();
    test_optimized_types_to_string<std::array<signed char, 1> >();
    test_optimized_types_to_string<std::array<signed char, 5> >();
    
    test_optimized_types_to_string_const<std::array<const char, 1> >();
    test_optimized_types_to_string_const<std::array<const char, 5> >();
    test_optimized_types_to_string_const<std::array<const unsigned char, 1> >();
    test_optimized_types_to_string_const<std::array<const unsigned char, 5> >();
    test_optimized_types_to_string_const<std::array<const signed char, 1> >();
    test_optimized_types_to_string_const<std::array<const signed char, 5> >();
#endif
}

boost::unit_test::test_suite *init_unit_test_suite(int, char *[])
{
    boost::unit_test::test_suite *suite =
        BOOST_TEST_SUITE("lexical_cast traits tests");
    suite->add(BOOST_TEST_CASE(&test_metafunctions));
    return suite;
}

