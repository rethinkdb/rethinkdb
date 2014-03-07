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

// keyword_p for C++
// (for basic usage instead of std_p)
const distinct_parser<> keyword_p("0-9a-zA-Z_");

// keyword_d for C++
// (for mor intricate usage, for example together with symbol tables)
const distinct_directive<> keyword_d("0-9a-zA-Z_");

struct my_grammar: public grammar<my_grammar>
{
    template <typename ScannerT>
    struct definition
    {
        typedef rule<ScannerT> rule_t;

        definition(my_grammar const& self)
        {
            top
                =
                    keyword_p("declare") // use keyword_p instead of std_p
                >>  !ch_p(':')
                >>  keyword_d[str_p("ident")] // use keyword_d
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

    info = parse("declare ident", gram, space_p);
    BOOST_ASSERT(info.full); // valid input

    info = parse("declare: ident", gram, space_p);
    BOOST_ASSERT(info.full); // valid input

    info = parse("declareident", gram, space_p);
    BOOST_ASSERT(!info.hit); // invalid input

    return exit_success;
}
