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

namespace boost { namespace spirit
{
    template <typename DerivedT>
    struct sub_grammar : parser<DerivedT>
    {
        typedef sub_grammar         self_t;
        typedef DerivedT const&     embed_t;

        template <typename ScannerT>
        struct result
        {
            typedef typename parser_result<
                typename DerivedT::start_t, ScannerT>::type
            type;
        };

        DerivedT const& derived() const
        { return *static_cast<DerivedT const*>(this); }

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            return derived().start.parse(scan);
        }
    };
}}

///////////////////////////////////////////////////////////////////////////////
//
//  Client code
//
///////////////////////////////////////////////////////////////////////////////
struct skip_grammar : boost::spirit::sub_grammar<skip_grammar>
{
    typedef
       alternative<alternative<space_parser, sequence<sequence<
       strlit<const char*>, kleene_star<difference<anychar_parser,
       chlit<char> > > >, chlit<char> > >, sequence<sequence<
       strlit<const char*>, kleene_star<difference<anychar_parser,
       strlit<const char*> > > >, strlit<const char*> > >
    start_t;

    skip_grammar()
    : start
        (
            space_p
        |   "//" >> *(anychar_p - '\n') >> '\n'
        |   "/*" >> *(anychar_p - "*/") >> "*/"
        )
    {}

    start_t start;
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
