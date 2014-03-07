//  Unit test for boost::lexical_cast.
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2011.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).

#include <boost/config.hpp>

#if defined(__INTEL_COMPILER)
#pragma warning(disable: 193 383 488 981 1418 1419)
#elif defined(BOOST_MSVC)
#pragma warning(disable: 4097 4100 4121 4127 4146 4244 4245 4511 4512 4701 4800)
#endif

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/range/iterator_range.hpp>

using namespace boost;

#if defined(BOOST_NO_STRINGSTREAM) || defined(BOOST_NO_STD_WSTRING)
#define BOOST_LCAST_NO_WCHAR_T
#endif

template <class T>
void do_test_on_empty_input(T& v)
{
    BOOST_CHECK_THROW(lexical_cast<int>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<float>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<double>(v), bad_lexical_cast);
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
    BOOST_CHECK_THROW(lexical_cast<long double>(v), bad_lexical_cast);
#endif
    BOOST_CHECK_THROW(lexical_cast<unsigned int>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<unsigned short>(v), bad_lexical_cast);
#if defined(BOOST_HAS_LONG_LONG)
    BOOST_CHECK_THROW(lexical_cast<boost::ulong_long_type>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<boost::long_long_type>(v), bad_lexical_cast);
#elif defined(BOOST_HAS_MS_INT64)
    BOOST_CHECK_THROW(lexical_cast<unsigned __int64>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<__int64>(v), bad_lexical_cast);
#endif
}

void test_empty_iterator_range()
{

    boost::iterator_range<char*> v;
    do_test_on_empty_input(v);
    BOOST_CHECK_EQUAL(lexical_cast<std::string>(v), std::string());
    BOOST_CHECK_THROW(lexical_cast<char>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<unsigned char>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<signed char>(v), bad_lexical_cast);

    boost::iterator_range<const char*> cv;
    do_test_on_empty_input(cv);
    BOOST_CHECK_EQUAL(lexical_cast<std::string>(cv), std::string());
    BOOST_CHECK_THROW(lexical_cast<char>(cv), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<unsigned char>(cv), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<signed char>(cv), bad_lexical_cast);

    const boost::iterator_range<const char*> ccv;
    do_test_on_empty_input(ccv);
    BOOST_CHECK_EQUAL(lexical_cast<std::string>(ccv), std::string());
    BOOST_CHECK_THROW(lexical_cast<char>(ccv), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<unsigned char>(ccv), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<signed char>(ccv), bad_lexical_cast);
}

void test_empty_string()
{
    std::string v;
    do_test_on_empty_input(v);
    BOOST_CHECK_THROW(lexical_cast<char>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<unsigned char>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<signed char>(v), bad_lexical_cast);

#ifndef BOOST_LCAST_NO_WCHAR_T
    std::wstring vw;
    do_test_on_empty_input(vw);
    BOOST_CHECK_THROW(lexical_cast<wchar_t>(vw), bad_lexical_cast);
#endif

// Currently, no compiler and STL library fully support char16_t and char32_t
//#ifndef BOOST_NO_CXX11_CHAR16_T
//    std::basic_string<char16_t> v16w;
//    do_test_on_empty_input(v16w);
//    BOOST_CHECK_THROW(lexical_cast<char16_t>(v16w), bad_lexical_cast);
//#endif
//#ifndef BOOST_NO_CXX11_CHAR32_T
//    std::basic_string<char32_t> v32w;
//    do_test_on_empty_input(v32w);
//    BOOST_CHECK_THROW(lexical_cast<char32_t>(v32w), bad_lexical_cast);
//#endif
}

struct Escape
{
    Escape(const std::string& s)
        : str_(s)
    {}

    std::string str_;
};

inline std::ostream& operator<< (std::ostream& o, const Escape& rhs)
{
    return o << rhs.str_;
}

void test_empty_user_class()
{
    Escape v("");
    do_test_on_empty_input(v);
    BOOST_CHECK_THROW(lexical_cast<char>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<unsigned char>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<signed char>(v), bad_lexical_cast);
}

namespace std {
inline std::ostream & operator<<(std::ostream & out, const std::vector<long> & v)
{
    std::ostream_iterator<long> it(out);
    std::copy(v.begin(), v.end(), it);
    assert(out);
    return out;
}
}

void test_empty_vector()
{
    std::vector<long> v;
    do_test_on_empty_input(v);
    BOOST_CHECK_THROW(lexical_cast<char>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<unsigned char>(v), bad_lexical_cast);
    BOOST_CHECK_THROW(lexical_cast<signed char>(v), bad_lexical_cast);
}


struct my_string {
    friend std::ostream &operator<<(std::ostream& sout, my_string const&/* st*/) {
            return sout << "";
    }
};

void test_empty_zero_terminated_string()
{
    my_string st;
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(st), std::string());;
}

unit_test::test_suite *init_unit_test_suite(int, char *[])
{
    unit_test::test_suite *suite =
        BOOST_TEST_SUITE("lexical_cast. Empty input unit test");
    suite->add(BOOST_TEST_CASE(&test_empty_iterator_range));
    suite->add(BOOST_TEST_CASE(&test_empty_string));
    suite->add(BOOST_TEST_CASE(&test_empty_user_class));
    suite->add(BOOST_TEST_CASE(&test_empty_vector));
    suite->add(BOOST_TEST_CASE(&test_empty_zero_terminated_string));

    return suite;
}
