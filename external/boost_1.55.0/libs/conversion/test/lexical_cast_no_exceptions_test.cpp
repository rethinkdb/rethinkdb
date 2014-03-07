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

#if defined(__INTEL_COMPILER)
#pragma warning(disable: 193 383 488 981 1418 1419)
#elif defined(BOOST_MSVC)
#pragma warning(disable: 4097 4100 4121 4127 4146 4244 4245 4511 4512 4701 4800)
#endif

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/range/iterator_range.hpp>

#ifndef BOOST_NO_EXCEPTIONS
#error "This test must be compiled with -DBOOST_NO_EXCEPTIONS"
#endif

bool g_was_exception = false;

namespace boost {

void throw_exception(std::exception const & ) {
    g_was_exception = true;
}

}

using namespace boost;


struct Escape
{
    Escape(){}
    Escape(const std::string& s)
        : str_(s)
    {}

    std::string str_;
};

inline std::ostream& operator<< (std::ostream& o, const Escape& rhs)
{
    return o << rhs.str_;
}

inline std::istream& operator>> (std::istream& i, Escape& rhs)
{
    return i >> rhs.str_;
}

void test_exceptions_off()
{
    Escape v("");

    g_was_exception = false;
    lexical_cast<char>(v);
    BOOST_CHECK(g_was_exception);

    g_was_exception = false;
    lexical_cast<unsigned char>(v);
    BOOST_CHECK(g_was_exception);

    v = lexical_cast<Escape>(100);
    BOOST_CHECK_EQUAL(lexical_cast<int>(v), 100);
    BOOST_CHECK_EQUAL(lexical_cast<unsigned int>(v), 100u);

    v = lexical_cast<Escape>(0.0);
    BOOST_CHECK_EQUAL(lexical_cast<double>(v), 0.0);

    BOOST_CHECK_EQUAL(lexical_cast<short>(100), 100);
    BOOST_CHECK_EQUAL(lexical_cast<float>(0.0), 0.0);

    g_was_exception = false;
    lexical_cast<short>(700000);
    BOOST_CHECK(g_was_exception);
}

unit_test::test_suite *init_unit_test_suite(int, char *[])
{
    unit_test::test_suite *suite =
        BOOST_TEST_SUITE("lexical_cast. Testing with BOOST_NO_EXCEPTIONS");
    suite->add(BOOST_TEST_CASE(&test_exceptions_off));

    return suite;
}

