/*=============================================================================
    Copyright (c) 2004 Stefan Slapeta
    Copyright (c) 2002-2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
// vi:ts=4:sw=4:et
// Tests for BOOST_SPIRIT_CLASSIC_NS::if_p
// [28-Dec-2002]
////////////////////////////////////////////////////////////////////////////////
#define qDebug 0
#include <iostream>
#include <cstring>
#if qDebug
#define BOOST_SPIRIT_DEBUG
#endif
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_if.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/ref.hpp>
#include "impl/string_length.hpp"

namespace local
{
    template <typename T>
    struct var_wrapper
        : public ::boost::reference_wrapper<T>
    {
        typedef ::boost::reference_wrapper<T> parent;

        explicit inline var_wrapper(T& t) : parent(t) {}

        inline T& operator()() const { return parent::get(); }
    };

    template <typename T>
    inline var_wrapper<T>
    var(T& t)
    {
        return var_wrapper<T>(t);
    }
}

typedef ::BOOST_SPIRIT_CLASSIC_NS::rule<> rule_t;
typedef ::BOOST_SPIRIT_CLASSIC_NS::rule<BOOST_SPIRIT_CLASSIC_NS::no_actions_scanner<>::type >
    no_actions_rule_t;

unsigned int test_count = 0;
unsigned int error_count = 0;

unsigned int number_result;
static const unsigned int kError = 999;
static const bool good = true;
static const bool bad = false;

rule_t hex_prefix;
no_actions_rule_t oct_prefix;
rule_t hex_rule, oct_rule, dec_rule;

rule_t auto_number_rule;
rule_t hex_or_dec_number_rule;

void
test_number(char const *s, unsigned int wanted, rule_t const &r)
{
    using namespace std;
    
    ++test_count;

    number_result = wanted-1;
    ::BOOST_SPIRIT_CLASSIC_NS::parse_info<> m = ::BOOST_SPIRIT_CLASSIC_NS::parse(s, s + test_impl::string_length(s), r);

    bool result = wanted == kError?(m.full?bad:good): (number_result==wanted);

    if (m.full && (m.length != test_impl::string_length(s)))
        result = bad;


    if (result==good)
        cout << "PASSED";
    else
    {
        ++error_count;
        cout << "FAILED";
    }

    cout << ": \"" << s << "\" ==> ";
    if (number_result==wanted-1)
        cout << "<error>";
    else
        cout << number_result;

    cout << "\n";
}

void
test_enclosed_fail()
{
    using namespace std;

    using ::BOOST_SPIRIT_CLASSIC_NS::if_p;
    using ::BOOST_SPIRIT_CLASSIC_NS::str_p;
    using ::BOOST_SPIRIT_CLASSIC_NS::nothing_p;

  cout << "\nfail enclosed parser:\n";

    const char *p = "abc";

    ::BOOST_SPIRIT_CLASSIC_NS::strlit<const char*> success_p = str_p(p);
    ::BOOST_SPIRIT_CLASSIC_NS::strlit<const char*> fail_p = str_p("xxx");

    ::BOOST_SPIRIT_CLASSIC_NS::rule<> r = if_p(success_p)[nothing_p];

    ::BOOST_SPIRIT_CLASSIC_NS::parse_info<> m = ::BOOST_SPIRIT_CLASSIC_NS::parse(p, r);

    if (m.full) {
        cout << "FAILED: if --> match" << endl;
        ++error_count;
    } else {
        cout << "PASSED: if --> no_match" << endl;
    }

    r = if_p(fail_p)[success_p].else_p[nothing_p];

    m = ::BOOST_SPIRIT_CLASSIC_NS::parse(p, r);

    if (m.full) {
        cout << "FAILED: else --> match" << endl;
        ++error_count;
    } else {
        cout << "PASSED: else --> no_match" << endl;
    }
}

int
main()
{
    using namespace std;
    using ::BOOST_SPIRIT_CLASSIC_NS::if_p;
    using ::BOOST_SPIRIT_CLASSIC_NS::uint_p;
    using ::BOOST_SPIRIT_CLASSIC_NS::oct_p;
    using ::BOOST_SPIRIT_CLASSIC_NS::hex_p;
    using ::BOOST_SPIRIT_CLASSIC_NS::str_p;
    using ::BOOST_SPIRIT_CLASSIC_NS::ch_p;
    using ::BOOST_SPIRIT_CLASSIC_NS::assign_a;

    cout << "/////////////////////////////////////////////////////////\n";
    cout << "\n";
    cout << "          if_p test\n";
    cout << "\n";
    cout << "/////////////////////////////////////////////////////////\n";
    cout << "\n";

    bool    as_hex;

#if qDebug
    BOOST_SPIRIT_DEBUG_RULE(hex_prefix);
    BOOST_SPIRIT_DEBUG_RULE(hex_rule);
    BOOST_SPIRIT_DEBUG_RULE(oct_prefix);
    BOOST_SPIRIT_DEBUG_RULE(oct_rule);
    BOOST_SPIRIT_DEBUG_RULE(dec_rule);
    BOOST_SPIRIT_DEBUG_RULE(auto_number_rule);
    BOOST_SPIRIT_DEBUG_RULE(hex_or_dec_number_rule);
#endif

    hex_prefix = str_p("0x");
    oct_prefix = ch_p('0');

    hex_rule = hex_p[assign_a(number_result)];
    oct_rule = oct_p[assign_a(number_result)];
    dec_rule = uint_p[assign_a(number_result)];

    auto_number_rule =
        if_p(hex_prefix)
            [hex_rule]
        .else_p
        [
            if_p(::BOOST_SPIRIT_CLASSIC_NS::eps_p(oct_prefix))
                [oct_rule]
            .else_p
                [dec_rule]
        ];

    hex_or_dec_number_rule =
        if_p(local::var(as_hex))[hex_prefix>>hex_rule].else_p[dec_rule];

    cout << "auto:\n";
    test_number("",   kError, auto_number_rule);
    test_number("0",       0, auto_number_rule);
    test_number("1",       1, auto_number_rule);
    test_number("00",      0, auto_number_rule);
    test_number("0x", kError, auto_number_rule);
    test_number("0x0",     0, auto_number_rule);
    test_number("0755",  493, auto_number_rule);
    test_number("0x100", 256, auto_number_rule);

    cout << "\ndecimal:\n";
    as_hex = false;
    test_number("",      kError, hex_or_dec_number_rule);
    test_number("100",      100, hex_or_dec_number_rule);
    test_number("0x100", kError, hex_or_dec_number_rule);
    test_number("0xff",  kError, hex_or_dec_number_rule);

    cout << "\nhexadecimal:\n";
    as_hex = true;
    test_number("",      kError, hex_or_dec_number_rule);
    test_number("0x100",    256, hex_or_dec_number_rule);
    test_number("0xff",     255, hex_or_dec_number_rule);

    //////////////////////////////////
    // tests for if_p without else-parser
    cout << "\nno-else:\n";
    rule_t r = if_p(::BOOST_SPIRIT_CLASSIC_NS::eps_p('0'))[oct_rule];

    test_number("0", 0, r);

    ++test_count;
    ::BOOST_SPIRIT_CLASSIC_NS::parse_info<> m = ::BOOST_SPIRIT_CLASSIC_NS::parse("", r);
    if (!m.hit || !m.full || m.length!=0)
    {
        std::cout << "FAILED: \"\" ==> <error>\n";
        ++error_count;
    }
    else
        std::cout << "PASSED: \"\" ==> <empty match>\n";

    ++test_count;
    m = ::BOOST_SPIRIT_CLASSIC_NS::parse("junk", r);
    if (!m.hit || m.full || m.length!=0)
    {
        std::cout << "FAILED: \"junk\" ==> <error>\n";
        ++error_count;
    }
    else
        std::cout << "PASSED: \"junk\" ==> <empty match>\n";

    test_enclosed_fail();


    //////////////////////////////////
    // report results
    std::cout << "\n    ";
    if (error_count==0)
        cout << "All " << test_count << " if_p-tests passed.\n"
             << "Test concluded successfully\n";
    else
        cout << error_count << " of " << test_count << " if_p-tests failed\n"
             << "Test failed\n";

    return error_count!=0;
}
