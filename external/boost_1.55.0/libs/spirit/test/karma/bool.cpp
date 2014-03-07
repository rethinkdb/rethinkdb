/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_phoenix_attributes.hpp>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
// special bool output policy allowing to spell false as true backwards (eurt)
struct special_bool_policy : boost::spirit::karma::bool_policies<>
{
    template <typename CharEncoding, typename Tag
      , typename OutputIterator>
    static bool generate_false(OutputIterator& sink, bool)
    {
        //  we want to spell the names of true and false backwards
        return boost::spirit::karma::string_inserter<CharEncoding, Tag>::
            call(sink, "eurt");
    }
};

///////////////////////////////////////////////////////////////////////////////
// special policy allowing to use any type as a boolean
struct test_bool_data
{
    explicit test_bool_data(bool b) : b(b) {}

    bool b;

    // we need to provide (safe) convertibility to bool
private:
    struct dummy { void true_() {} };
    typedef void (dummy::*safe_bool)();

public:
    operator safe_bool () const { return b ? &dummy::true_ : 0; }
};

struct test_bool_policy : boost::spirit::karma::bool_policies<>
{
    template <typename Inserter, typename OutputIterator, typename Policies>
    static bool
    call (OutputIterator& sink, test_bool_data b, Policies const& p)
    {
        //  call the predefined inserter to do the job
        return Inserter::call_n(sink, bool(b), p);
    }
};


///////////////////////////////////////////////////////////////////////////////
int main()
{
    using boost::spirit::karma::bool_;
    using boost::spirit::karma::false_;
    using boost::spirit::karma::true_;
    using boost::spirit::karma::lit;
    using boost::spirit::karma::lower;
    using boost::spirit::karma::upper;

    // testing plain bool
    {
        BOOST_TEST(test("false", bool_, false));
        BOOST_TEST(test("true", bool_, true));
        BOOST_TEST(test("false", false_, false));
        BOOST_TEST(test("true", true_, true));
        BOOST_TEST(test("false", bool_(false)));
        BOOST_TEST(test("true", bool_(true)));
        BOOST_TEST(test("false", bool_(false), false));
        BOOST_TEST(test("true", bool_(true), true));
        BOOST_TEST(!test("", bool_(false), true));
        BOOST_TEST(!test("", bool_(true), false));
        BOOST_TEST(test("false", lit(false)));
        BOOST_TEST(test("true", lit(true)));
    }

    // test optional attributes
    {
        boost::optional<bool> optbool;

        BOOST_TEST(!test("", bool_, optbool));
        BOOST_TEST(test("", -bool_, optbool));
        optbool = false;
        BOOST_TEST(test("false", bool_, optbool));
        BOOST_TEST(test("false", -bool_, optbool));
        optbool = true;
        BOOST_TEST(test("true", bool_, optbool));
        BOOST_TEST(test("true", -bool_, optbool));
    }

// we support Phoenix attributes only starting with V2.2
#if SPIRIT_VERSION >= 0x2020
    // test Phoenix expression attributes (requires to include 
    // karma_phoenix_attributes.hpp)
    {
        namespace phoenix = boost::phoenix;

        BOOST_TEST(test("true", bool_, phoenix::val(true)));

        bool b = false;
        BOOST_TEST(test("false", bool_, phoenix::ref(b)));
        BOOST_TEST(test("true", bool_, ++phoenix::ref(b)));
    }
#endif

    {
        BOOST_TEST(test("false", lower[bool_], false));
        BOOST_TEST(test("true", lower[bool_], true));
        BOOST_TEST(test("false", lower[bool_(false)]));
        BOOST_TEST(test("true", lower[bool_(true)]));
        BOOST_TEST(test("false", lower[bool_(false)], false));
        BOOST_TEST(test("true", lower[bool_(true)], true));
        BOOST_TEST(!test("", lower[bool_(false)], true));
        BOOST_TEST(!test("", lower[bool_(true)], false));
        BOOST_TEST(test("false", lower[lit(false)]));
        BOOST_TEST(test("true", lower[lit(true)]));
    }

    {
        BOOST_TEST(test("FALSE", upper[bool_], false));
        BOOST_TEST(test("TRUE", upper[bool_], true));
        BOOST_TEST(test("FALSE", upper[bool_(false)]));
        BOOST_TEST(test("TRUE", upper[bool_(true)]));
        BOOST_TEST(test("FALSE", upper[bool_(false)], false));
        BOOST_TEST(test("TRUE", upper[bool_(true)], true));
        BOOST_TEST(!test("", upper[bool_(false)], true));
        BOOST_TEST(!test("", upper[bool_(true)], false));
        BOOST_TEST(test("FALSE", upper[lit(false)]));
        BOOST_TEST(test("TRUE", upper[lit(true)]));
    }

    {
        typedef boost::spirit::karma::bool_generator<bool, special_bool_policy> 
            backwards_bool_type;
        backwards_bool_type const backwards_bool = backwards_bool_type();

        BOOST_TEST(test("eurt", backwards_bool, false));
        BOOST_TEST(test("true", backwards_bool, true));
        BOOST_TEST(test("eurt", backwards_bool(false)));
        BOOST_TEST(test("true", backwards_bool(true)));
        BOOST_TEST(test("eurt", backwards_bool(false), false));
        BOOST_TEST(test("true", backwards_bool(true), true));
        BOOST_TEST(!test("", backwards_bool(false), true));
        BOOST_TEST(!test("", backwards_bool(true), false));
    }

    {
        typedef boost::spirit::karma::bool_generator<
            test_bool_data, test_bool_policy> test_bool_type;
        test_bool_type const test_bool = test_bool_type();

        test_bool_data const test_false = test_bool_data(false);
        test_bool_data const test_true = test_bool_data(true);

        BOOST_TEST(test("false", test_bool, test_false));
        BOOST_TEST(test("true", test_bool, test_true));
        BOOST_TEST(test("false", test_bool(test_false)));
        BOOST_TEST(test("true", test_bool(test_true)));
        BOOST_TEST(test("false", test_bool(test_false), test_false));
        BOOST_TEST(test("true", test_bool(test_true), test_true));
        BOOST_TEST(!test("", test_bool(test_false), test_true));
        BOOST_TEST(!test("", test_bool(test_true), test_false));
    }

    return boost::report_errors();
}

