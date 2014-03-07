/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// *** See the section "Look Ma' No Rules" in
// *** chapter "Techniques" of the Spirit documentation
// *** for information regarding this snippet

#include <iostream>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/assert.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;

    struct skip_grammar : grammar<skip_grammar>
    {
        template <typename ScannerT>
        struct definition
        {
            definition(skip_grammar const& /*self*/)
            {
                skip
                    =   space_p
                    |   "//" >> *(anychar_p - '\n') >> '\n'
                    |   "/*" >> *(anychar_p - "*/") >> "*/"
                    ;
            }

            rule<ScannerT> skip;

            rule<ScannerT> const&
            start() const { return skip; }
        };
    };

int
main()
{
    skip_grammar g;
    bool success = parse(
        "/*this is a comment*/\n//this is a c++ comment\n\n",
        *g).full;
    BOOST_ASSERT(success);
    std::cout << "SUCCESS!!!\n";
    return 0;
}
