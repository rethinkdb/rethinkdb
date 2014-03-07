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

using namespace boost;

#if defined(BOOST_NO_STRINGSTREAM)
            typedef std::strstream ss_t;
#else
            typedef std::stringstream ss_t;
#endif

void test_void_pointers_conversions()
{
    void *p_to_null = NULL;
    const void *cp_to_data = "Some data";
    char nonconst_data[5];
    void *p_to_data = nonconst_data;
    ss_t ss;

    ss << p_to_null;
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_to_null), ss.str());
    ss.str(std::string());

    ss << cp_to_data;
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(cp_to_data), ss.str());
    ss.str(std::string());

    ss << p_to_data;
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_to_data), ss.str());
    ss.str(std::string());
}

struct incomplete_type;

void test_incomplete_type_pointers_conversions()
{
    incomplete_type *p_to_null = NULL;
    const incomplete_type *cp_to_data = NULL;
    char nonconst_data[5];
    incomplete_type *p_to_data = reinterpret_cast<incomplete_type*>(nonconst_data);
    ss_t ss;

    ss << p_to_null;
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_to_null), ss.str());
    ss.str(std::string());

    ss << cp_to_data;
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(cp_to_data), ss.str());
    ss.str(std::string());

    ss << p_to_data;
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(p_to_data), ss.str());
    ss.str(std::string());
}

struct ble;
typedef struct ble *meh;
std::ostream& operator <<(std::ostream &o, meh) {
  o << "yay";
  return o;
}

void test_inomplete_type_with_overloaded_ostream_op() {
    meh heh = NULL;
    ss_t ss;
    ss << heh;
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(heh), ss.str());
}

unit_test::test_suite *init_unit_test_suite(int, char *[])
{
    unit_test::test_suite *suite =
        BOOST_TEST_SUITE("lexical_cast pinters test");
    suite->add(BOOST_TEST_CASE(&test_void_pointers_conversions));
    suite->add(BOOST_TEST_CASE(&test_incomplete_type_pointers_conversions));
    suite->add(BOOST_TEST_CASE(&test_inomplete_type_with_overloaded_ostream_op));
    return suite;
}
