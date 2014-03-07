//  Testing boost::lexical_cast with boost::container::string.
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2012.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).

#include <boost/lexical_cast.hpp>

#include <boost/test/unit_test.hpp>
    
#include <boost/array.hpp>

void testing_boost_array_output_conversion();
void testing_std_array_output_conversion();

void testing_boost_array_input_conversion();
void testing_std_array_input_conversion();

using namespace boost;

#if !defined(BOOST_NO_CXX11_CHAR16_T) && !defined(BOOST_NO_CXX11_UNICODE_LITERALS) && !defined(_LIBCPP_VERSION)
#define BOOST_LC_RUNU16
#endif

#if !defined(BOOST_NO_CXX11_CHAR32_T) && !defined(BOOST_NO_CXX11_UNICODE_LITERALS) && !defined(_LIBCPP_VERSION)
#define BOOST_LC_RUNU32
#endif

boost::unit_test::test_suite *init_unit_test_suite(int, char *[])
{
    unit_test::test_suite *suite =
        BOOST_TEST_SUITE("Testing boost::lexical_cast with boost::array and std::array");

    suite->add(BOOST_TEST_CASE(testing_boost_array_output_conversion));
    suite->add(BOOST_TEST_CASE(testing_std_array_output_conversion));
    suite->add(BOOST_TEST_CASE(testing_boost_array_input_conversion));
    suite->add(BOOST_TEST_CASE(testing_std_array_input_conversion));

    return suite;
}

template <template <class, std::size_t> class ArrayT, class T>
static void testing_template_array_output_on_spec_value(T val) 
{
    typedef ArrayT<char, 300>             arr_type;
    typedef ArrayT<char, 1>               short_arr_type;
    typedef ArrayT<unsigned char, 300>    uarr_type;
    typedef ArrayT<unsigned char, 1>      ushort_arr_type;
    typedef ArrayT<signed char, 4>        sarr_type;
    typedef ArrayT<signed char, 3>        sshort_arr_type;

    std::string ethalon("100");
    using namespace std;

    {
        arr_type res1 = lexical_cast<arr_type>(val);
        BOOST_CHECK_EQUAL(&res1[0], ethalon);
        const arr_type res2 = lexical_cast<arr_type>(val);
        BOOST_CHECK_EQUAL(&res2[0], ethalon);
        BOOST_CHECK_THROW(lexical_cast<short_arr_type>(val), boost::bad_lexical_cast);
    }
    
    {
        uarr_type res1 = lexical_cast<uarr_type>(val);
        BOOST_CHECK_EQUAL(reinterpret_cast<char*>(&res1[0]), ethalon);
        const uarr_type res2 = lexical_cast<uarr_type>(val);
        BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(&res2[0]), ethalon);
        BOOST_CHECK_THROW(lexical_cast<ushort_arr_type>(val), boost::bad_lexical_cast);
    }
    
    {
        sarr_type res1 = lexical_cast<sarr_type>(val);
        BOOST_CHECK_EQUAL(reinterpret_cast<char*>(&res1[0]), ethalon);
        const sarr_type res2 = lexical_cast<sarr_type>(val);
        BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(&res2[0]), ethalon);
        BOOST_CHECK_THROW(lexical_cast<sshort_arr_type>(val), boost::bad_lexical_cast);
    }

#if !defined(BOOST_NO_STRINGSTREAM) && !defined(BOOST_NO_STD_WSTRING)
    typedef ArrayT<wchar_t, 300> warr_type;
    typedef ArrayT<wchar_t, 3> wshort_arr_type;
    std::wstring wethalon(L"100");

    {
        warr_type res = lexical_cast<warr_type>(val);
        BOOST_CHECK(&res[0] == wethalon);
    }

    {
        const warr_type res = lexical_cast<warr_type>(val);
        BOOST_CHECK(&res[0] == wethalon);
    }
    
    BOOST_CHECK_THROW(lexical_cast<wshort_arr_type>(val), boost::bad_lexical_cast);

#endif

#ifdef BOOST_LC_RUNU16
    typedef ArrayT<char16_t, 300> u16arr_type;
    typedef ArrayT<char16_t, 3> u16short_arr_type;
    std::u16string u16ethalon(u"100");

    {
        u16arr_type res = lexical_cast<u16arr_type>(val);
        BOOST_CHECK(&res[0] == u16ethalon);
    }

    {
        const u16arr_type res = lexical_cast<u16arr_type>(val);
        BOOST_CHECK(&res[0] == u16ethalon);
    }
    
    BOOST_CHECK_THROW(lexical_cast<u16short_arr_type>(val), boost::bad_lexical_cast);
#endif

#ifdef BOOST_LC_RUNU32
    typedef ArrayT<char32_t, 300> u32arr_type;
    typedef ArrayT<char32_t, 3> u32short_arr_type;
    std::u32string u32ethalon(U"100");

    {
        u32arr_type res = lexical_cast<u32arr_type>(val);
        BOOST_CHECK(&res[0] == u32ethalon);
    }

    {
        const u32arr_type res = lexical_cast<u32arr_type>(val);
        BOOST_CHECK(&res[0] == u32ethalon);
    }
    
    BOOST_CHECK_THROW(lexical_cast<u32short_arr_type>(val), boost::bad_lexical_cast);
#endif
}


template <template <class, std::size_t> class ArrayT>
static void testing_template_array_output_on_char_value() 
{
    typedef ArrayT<char, 300>             arr_type;
    typedef ArrayT<char, 1>               short_arr_type;
    typedef ArrayT<unsigned char, 300>    uarr_type;
    typedef ArrayT<unsigned char, 1>      ushort_arr_type;
    typedef ArrayT<signed char, 4>        sarr_type;
    typedef ArrayT<signed char, 3>        sshort_arr_type;

    const char val[] = "100";
    std::string ethalon("100");
    using namespace std;

    {
        arr_type res1 = lexical_cast<arr_type>(val);
        BOOST_CHECK_EQUAL(&res1[0], ethalon);
        const arr_type res2 = lexical_cast<arr_type>(val);
        BOOST_CHECK_EQUAL(&res2[0], ethalon);
        BOOST_CHECK_THROW(lexical_cast<short_arr_type>(val), boost::bad_lexical_cast);
    }
    
    {
        uarr_type res1 = lexical_cast<uarr_type>(val);
        BOOST_CHECK_EQUAL(reinterpret_cast<char*>(&res1[0]), ethalon);
        const uarr_type res2 = lexical_cast<uarr_type>(val);
        BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(&res2[0]), ethalon);
        BOOST_CHECK_THROW(lexical_cast<ushort_arr_type>(val), boost::bad_lexical_cast);
    }
    
    {
        sarr_type res1 = lexical_cast<sarr_type>(val);
        BOOST_CHECK_EQUAL(reinterpret_cast<char*>(&res1[0]), ethalon);
        const sarr_type res2 = lexical_cast<sarr_type>(val);
        BOOST_CHECK_EQUAL(reinterpret_cast<const char*>(&res2[0]), ethalon);
        BOOST_CHECK_THROW(lexical_cast<sshort_arr_type>(val), boost::bad_lexical_cast);
    }

#if !defined(BOOST_NO_STRINGSTREAM) && !defined(BOOST_NO_STD_WSTRING)
    typedef ArrayT<wchar_t, 4> warr_type;
    typedef ArrayT<wchar_t, 3> wshort_arr_type;
    std::wstring wethalon(L"100");

    {
        warr_type res = lexical_cast<warr_type>(val);
        BOOST_CHECK(&res[0] == wethalon);
        warr_type res3 = lexical_cast<warr_type>(wethalon);
        BOOST_CHECK(&res3[0] == wethalon);
    }

    {
        const warr_type res = lexical_cast<warr_type>(val);
        BOOST_CHECK(&res[0] == wethalon);
        const warr_type res3 = lexical_cast<warr_type>(wethalon);
        BOOST_CHECK(&res3[0] == wethalon);
    }
    
    BOOST_CHECK_THROW(lexical_cast<wshort_arr_type>(val), boost::bad_lexical_cast);

#endif

#ifdef BOOST_LC_RUNU16
    typedef ArrayT<char16_t, 300> u16arr_type;
    typedef ArrayT<char16_t, 3> u16short_arr_type;
    std::u16string u16ethalon(u"100");

    {
#ifdef BOOST_STL_SUPPORTS_NEW_UNICODE_LOCALES
        u16arr_type res = lexical_cast<u16arr_type>(val);
        BOOST_CHECK(&res[0] == u16ethalon);  
#endif

        u16arr_type res3 = lexical_cast<u16arr_type>(u16ethalon);
        BOOST_CHECK(&res3[0] == u16ethalon);
    }

    {
#ifdef BOOST_STL_SUPPORTS_NEW_UNICODE_LOCALES
        const u16arr_type res = lexical_cast<u16arr_type>(val);
        BOOST_CHECK(&res[0] == u16ethalon);
#endif
        const u16arr_type res3 = lexical_cast<u16arr_type>(u16ethalon);
        BOOST_CHECK(&res3[0] == u16ethalon);
    }
    
    // Some compillers may throw std::bad_alloc here
    BOOST_CHECK_THROW(lexical_cast<u16short_arr_type>(val), std::exception); 
#endif

#ifdef BOOST_LC_RUNU32
    typedef ArrayT<char32_t, 300> u32arr_type;
    typedef ArrayT<char32_t, 3> u32short_arr_type;
    std::u32string u32ethalon(U"100");

    {
#ifdef BOOST_STL_SUPPORTS_NEW_UNICODE_LOCALES
        u32arr_type res = lexical_cast<u32arr_type>(val);
        BOOST_CHECK(&res[0] == u32ethalon);
#endif
        u32arr_type res3 = lexical_cast<u32arr_type>(u32ethalon);
        BOOST_CHECK(&res3[0] == u32ethalon);
    }

    {
#ifdef BOOST_STL_SUPPORTS_NEW_UNICODE_LOCALES
        const u32arr_type res = lexical_cast<u32arr_type>(val);
        BOOST_CHECK(&res[0] == u32ethalon);
#endif
        const u32arr_type res3 = lexical_cast<u32arr_type>(u32ethalon);
        BOOST_CHECK(&res3[0] == u32ethalon);
    }
    
    // Some compillers may throw std::bad_alloc here
    BOOST_CHECK_THROW(lexical_cast<u32short_arr_type>(val), std::exception);
#endif
}


void testing_boost_array_output_conversion()
{   
    testing_template_array_output_on_char_value<boost::array>();
    testing_template_array_output_on_spec_value<boost::array>(100);
    testing_template_array_output_on_spec_value<boost::array>(static_cast<short>(100));
    testing_template_array_output_on_spec_value<boost::array>(static_cast<unsigned short>(100));
    testing_template_array_output_on_spec_value<boost::array>(static_cast<unsigned int>(100));
}

void testing_std_array_output_conversion()
{
#ifndef BOOST_NO_CXX11_HDR_ARRAY
    testing_template_array_output_on_char_value<std::array>();
    testing_template_array_output_on_spec_value<std::array>(100);
    testing_template_array_output_on_spec_value<std::array>(static_cast<short>(100));
    testing_template_array_output_on_spec_value<std::array>(static_cast<unsigned short>(100));
    testing_template_array_output_on_spec_value<std::array>(static_cast<unsigned int>(100));
#endif

    BOOST_CHECK(true);
}

template <template <class, std::size_t> class ArrayT>
static void testing_generic_array_input_conversion()
{
    {
        ArrayT<char, 4> var_zero_terminated = {{ '1', '0', '0', '\0'}}; 
        BOOST_CHECK_EQUAL(lexical_cast<std::string>(var_zero_terminated), "100");
        BOOST_CHECK_EQUAL(lexical_cast<int>(var_zero_terminated), 100);

        ArrayT<char, 3> var_none_terminated = {{ '1', '0', '0'}}; 
        BOOST_CHECK_EQUAL(lexical_cast<std::string>(var_none_terminated), "100");
        BOOST_CHECK_EQUAL(lexical_cast<short>(var_none_terminated), static_cast<short>(100));

        ArrayT<const char, 4> var_zero_terminated_const_char = {{ '1', '0', '0', '\0'}}; 
        BOOST_CHECK_EQUAL(lexical_cast<std::string>(var_zero_terminated_const_char), "100");

        ArrayT<const char, 3> var_none_terminated_const_char = {{ '1', '0', '0'}}; 
        BOOST_CHECK_EQUAL(lexical_cast<std::string>(var_none_terminated_const_char), "100");

        const ArrayT<char, 4> var_zero_terminated_const_var = {{ '1', '0', '0', '\0'}}; 
        BOOST_CHECK_EQUAL(lexical_cast<std::string>(var_zero_terminated_const_var), "100");

        const ArrayT<char, 3> var_none_terminated_const_var = {{ '1', '0', '0'}}; 
        BOOST_CHECK_EQUAL(lexical_cast<std::string>(var_none_terminated_const_var), "100");

        const ArrayT<const char, 4> var_zero_terminated_const_var_const_char = {{ '1', '0', '0', '\0'}}; 
        BOOST_CHECK_EQUAL(lexical_cast<std::string>(var_zero_terminated_const_var_const_char), "100");

        const ArrayT<const char, 3> var_none_terminated_const_var_const_char = {{ '1', '0', '0'}}; 
        BOOST_CHECK_EQUAL(lexical_cast<std::string>(var_none_terminated_const_var_const_char), "100");
        BOOST_CHECK_EQUAL(lexical_cast<int>(var_none_terminated_const_var_const_char), 100);
    }

    {
        const ArrayT<const unsigned char, 4> var_zero_terminated_const_var_const_char = {{ '1', '0', '0', '\0'}}; 
        BOOST_CHECK_EQUAL(lexical_cast<std::string>(var_zero_terminated_const_var_const_char), "100");

        const ArrayT<const unsigned char, 3> var_none_terminated_const_var_const_char = {{ '1', '0', '0'}}; 
        BOOST_CHECK_EQUAL(lexical_cast<std::string>(var_none_terminated_const_var_const_char), "100");
    }

    {
        const ArrayT<const signed char, 4> var_zero_terminated_const_var_const_char = {{ '1', '0', '0', '\0'}}; 
        BOOST_CHECK_EQUAL(lexical_cast<std::string>(var_zero_terminated_const_var_const_char), "100");

        const ArrayT<const signed char, 3> var_none_terminated_const_var_const_char = {{ '1', '0', '0'}}; 
        BOOST_CHECK_EQUAL(lexical_cast<std::string>(var_none_terminated_const_var_const_char), "100");
        BOOST_CHECK_EQUAL(lexical_cast<unsigned int>(var_none_terminated_const_var_const_char), 100u);
    }


#if !defined(BOOST_NO_STRINGSTREAM) && !defined(BOOST_NO_STD_WSTRING)
    {
        const ArrayT<const wchar_t, 4> var_zero_terminated_const_var_const_char = {{ L'1', L'0', L'0', L'\0'}}; 
        BOOST_CHECK(lexical_cast<std::wstring>(var_zero_terminated_const_var_const_char) == L"100");

        const ArrayT<const wchar_t, 3> var_none_terminated_const_var_const_char = {{ L'1', L'0', L'0'}}; 
        BOOST_CHECK(lexical_cast<std::wstring>(var_none_terminated_const_var_const_char) == L"100");
        BOOST_CHECK_EQUAL(lexical_cast<int>(var_none_terminated_const_var_const_char), 100);
    }
#endif

#ifdef BOOST_LC_RUNU16
    {
        const ArrayT<const char16_t, 4> var_zero_terminated_const_var_const_char = {{ u'1', u'0', u'0', u'\0'}}; 
        BOOST_CHECK(lexical_cast<std::u16string>(var_zero_terminated_const_var_const_char) == u"100");
        BOOST_CHECK_EQUAL(lexical_cast<unsigned short>(var_zero_terminated_const_var_const_char), static_cast<unsigned short>(100));

        const ArrayT<const char16_t, 3> var_none_terminated_const_var_const_char = {{ u'1', u'0', u'0'}}; 
        BOOST_CHECK(lexical_cast<std::u16string>(var_none_terminated_const_var_const_char) == u"100");
    }
#endif

#ifdef BOOST_LC_RUNU32
    {
        const ArrayT<const char32_t, 4> var_zero_terminated_const_var_const_char = {{ U'1', U'0', U'0', U'\0'}}; 
        BOOST_CHECK(lexical_cast<std::u32string>(var_zero_terminated_const_var_const_char) == U"100");

        const ArrayT<const char32_t, 3> var_none_terminated_const_var_const_char = {{ U'1', U'0', U'0'}}; 
        BOOST_CHECK(lexical_cast<std::u32string>(var_none_terminated_const_var_const_char) == U"100");
        BOOST_CHECK_EQUAL(lexical_cast<int>(var_none_terminated_const_var_const_char), 100);
    }
#endif
}

void testing_boost_array_input_conversion()
{
    testing_generic_array_input_conversion<boost::array>();
}

void testing_std_array_input_conversion()
{
#ifndef BOOST_NO_CXX11_HDR_ARRAY
    testing_generic_array_input_conversion<std::array>();
#endif

    BOOST_CHECK(true);
}

