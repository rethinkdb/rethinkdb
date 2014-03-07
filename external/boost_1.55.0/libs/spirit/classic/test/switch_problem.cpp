/*=============================================================================
    Copyright (c) 2004 Angus Leeming
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

///////////////////////////////////////////////////////////////////////////////
//
// The switch_p parser was broken sometime during the boost 1.32 development
// cycle. This little program tests it, the for_p parser and the limit_d
// directive.
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_for.hpp>
#include <boost/spirit/include/classic_switch.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/classic_confix.hpp>

#include <boost/spirit/include/phoenix1.hpp>

#include <iostream>
#include <string>

namespace spirit = BOOST_SPIRIT_CLASSIC_NS;

typedef unsigned int uint;

struct switch_grammar : public spirit::grammar<switch_grammar> {
    template <typename ScannerT>
    struct definition {
        definition(switch_grammar const & self);

        typedef spirit::rule<ScannerT> rule_t;
        rule_t const & start() const { return expression; }

    private:
        rule_t expression;
        uint index;
        uint nnodes;
    };
};


template <typename ScannerT>
switch_grammar::definition<ScannerT>::definition(switch_grammar const & self)
{
    using boost::cref;

    using phoenix::arg1;
    using phoenix::var;

    using spirit::case_p;
    using spirit::for_p;
    using spirit::limit_d;
    using spirit::str_p;
    using spirit::switch_p;
    using spirit::uint_p;

    expression =
        str_p("NNODES") >>
        uint_p[var(nnodes) = arg1] >>

        for_p(var(index) = 1,
              var(index) <= var(nnodes),
              var(index)++)
        [
            limit_d(cref(index), cref(index))[uint_p] >>

            switch_p[
                case_p<'s'>(uint_p),
                case_p<'d'>(uint_p),
                case_p<'n'>(uint_p)
            ]
        ];
}


int main()
{
    std::string const data("NNODES 3\n"
                   "1 s 1\n"
                   "2 d 2\n"
                   "3 n 3"); // JDG 10-18-2005 removed trailing \n to
                             // avoid post skip problems

    typedef spirit::position_iterator<std::string::const_iterator>
        iterator_t;

    spirit::parse_info<iterator_t> const info =
        parse(iterator_t(data.begin(), data.end(), "switch test"),
              iterator_t(),
              switch_grammar(),
              spirit::space_p);

    if (!info.full) {
        spirit::file_position const fp = info.stop.get_position();
        std::cerr << "Parsing failed at line " << fp.line
              << ", column " << fp.column << ".\n";
    }

    return info.full ? 0 : 1;
}
