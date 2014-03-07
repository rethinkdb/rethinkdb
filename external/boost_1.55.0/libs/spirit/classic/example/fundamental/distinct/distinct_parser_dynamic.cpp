/*=============================================================================
    Copyright (c) 2003 Vaclav Vesely
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/assert.hpp>
#include <iostream>
#include <boost/cstdlib.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_distinct.hpp>

using namespace std;
using namespace boost;
using namespace BOOST_SPIRIT_CLASSIC_NS;

struct my_grammar: public grammar<my_grammar>
{
    template <typename ScannerT>
    struct definition
    {
        typedef rule<ScannerT> rule_t;

        // keyword_p for ASN.1
        dynamic_distinct_parser<ScannerT> keyword_p;

        definition(my_grammar const& self)
        :   keyword_p(alnum_p | ('-' >> ~ch_p('-'))) // ASN.1 has quite complex naming rules
        {
            top
                =
                    keyword_p("asn-declare") // use keyword_p instead of std_p
                >>  !str_p("--")
                >>  keyword_p("ident")
                ;
        }

        rule_t top;

        rule_t const& start() const
        {
            return top;
        }
    };
};

int main()
{
    my_grammar gram;
    parse_info<> info;

    info = parse("asn-declare ident", gram, space_p);
    BOOST_ASSERT(info.full); // valid input

    info = parse("asn-declare--ident", gram, space_p);
    BOOST_ASSERT(info.full); // valid input

    info = parse("asn-declare-ident", gram, space_p);
    BOOST_ASSERT(!info.hit); // invalid input

    return exit_success;
}
