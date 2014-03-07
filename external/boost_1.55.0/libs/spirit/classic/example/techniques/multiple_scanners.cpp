/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// *** See the section "Rule With Multiple Scanners" in
// *** chapter "Techniques" of the Spirit documentation
// *** for information regarding this snippet

#define BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT 3

#include <iostream>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/assert.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;

struct my_grammar : grammar<my_grammar>
{
    template <typename ScannerT>
    struct definition
    {
        definition(my_grammar const& self)
        {
            r = lower_p;
            rr = +(lexeme_d[r] >> as_lower_d[r] >> r);
        }

        typedef scanner_list<
            ScannerT
          , typename lexeme_scanner<ScannerT>::type
          , typename as_lower_scanner<ScannerT>::type
        > scanners;

        rule<scanners> r;
        rule<ScannerT> rr;
        rule<ScannerT> const& start() const { return rr; }
    };
};

int
main()
{
    my_grammar g;
    bool success = parse("abcdef aBc d e f aBc d E f", g, space_p).full;
    BOOST_ASSERT(success);
    std::cout << "SUCCESS!!!\n";
    return 0;
}
