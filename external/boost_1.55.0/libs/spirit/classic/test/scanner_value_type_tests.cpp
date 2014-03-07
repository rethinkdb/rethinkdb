/*=============================================================================
    Copyright (c) 2005 Jordan DeLong
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Some tests of parsing on value_t's that aren't char or wchar_t.
//
// Part of what this is testing is that BOOST_SPIRIT_DEBUG doesn't
// break when the scanner::value_t is some sort of non-char.
#define SPIRIT_DEBUG_NODE

#include <boost/spirit/include/classic_core.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <deque>
#include <iostream>

namespace sp = BOOST_SPIRIT_CLASSIC_NS;

namespace {

    struct grammar : sp::grammar<grammar> {
        template<class Scanner>
        struct definition {
            definition(grammar const&)
            {
                foo
                    = sp::alpha_p
                      | sp::alnum_p
                      | sp::cntrl_p
                      | sp::print_p
                      | sp::blank_p
                      | sp::digit_p
                      | sp::graph_p
                      | sp::lower_p
                      | sp::upper_p
                      | sp::xdigit_p
                      | sp::punct_p
                    ;
            }

            sp::rule<Scanner> foo;
            sp::rule<Scanner> const&
            start() const
            {
                return foo;
            }
        };
    };

    struct non_pod {
        non_pod() : value('1') {}
        char value;
        bool operator==(non_pod const& o) const { return value == o.value; }
    };
    
    std::ostream&
    operator<<(std::ostream& out, non_pod const& x)
    {
        out << x.value;
        return out;
    }

    template<class T>
    struct convertable_non_pod : non_pod {
        operator T() { return value; }
    };

    struct nonpod_gram : sp::grammar<nonpod_gram> {
        template<class Scanner>
        struct definition {
            definition(nonpod_gram const&)
            {
                foo = sp::ch_p(typename Scanner::value_t());
            }

            sp::rule<Scanner> foo;
            sp::rule<Scanner> const&
            start() const
            {
                return foo;
            }
        };
    };

    template<class Grammar, class ValueType>
    void
    test_type()
    {
        typedef std::deque<ValueType> container;
        typedef typename container::iterator iterator;
        typedef sp::scanner<iterator> scanner;

        container blah;
        blah.push_back(typename container::value_type());
        blah.push_back(typename container::value_type());

        iterator first = blah.begin();
        iterator last = blah.end();

        scanner scan(first, last);

        // Make sure this actually tries what we think it tries.
        BOOST_STATIC_ASSERT((
            boost::is_same<typename scanner::value_t, ValueType>::value
        ));

        Grammar().parse(scan);
    }

}

int
main()
{
    // Make sure isfoo() style functions work for integral types.
    test_type<grammar, char>();
    test_type<grammar, unsigned char>();
    test_type<grammar, wchar_t>();
    test_type<grammar, int>();
    test_type<grammar, long>();
    test_type<grammar, short>();
    test_type<grammar, bool>();

    // Non-POD's should work with things like alpha_p as long as we
    // can turn them into a type that can do isalpha().
    test_type<grammar, convertable_non_pod<char> >();
    test_type<grammar, convertable_non_pod<wchar_t> >();
    test_type<grammar, convertable_non_pod<int> >();

    // BOOST_SPIRIT_DEBUG should work with grammars that parse
    // non-POD's even if they can't do things like isalpha().
    test_type<nonpod_gram, non_pod>();
}
