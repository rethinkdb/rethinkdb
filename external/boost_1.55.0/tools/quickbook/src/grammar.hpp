/*=============================================================================
    Copyright (c) 2002 2004  2006Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_QUICKBOOK_GRAMMARS_HPP)
#define BOOST_SPIRIT_QUICKBOOK_GRAMMARS_HPP

#include <boost/spirit/include/classic_core.hpp>
#include "fwd.hpp"

namespace quickbook
{
    namespace cl = boost::spirit::classic;

    // The spirit scanner for explicitly instantiating grammars. This is a
    // spirit implementation detail, but since classic is no longer under
    // development, it won't change. And spirit 2 won't require such a hack.

    typedef cl::scanner<parse_iterator, cl::scanner_policies <
        cl::iteration_policy, cl::match_policy, cl::action_policy> > scanner;

    template <typename Scanner>
    struct Scanner_must_be_the_quickbook_scanner_typedef;
    template <>
    struct Scanner_must_be_the_quickbook_scanner_typedef<scanner> {};

    struct grammar
        : public cl::grammar<grammar>
    {
        grammar(cl::rule<scanner> const& start_rule, char const* /* name */)
            : start_rule(start_rule) {}

        template <typename Scanner>
        struct definition :
            Scanner_must_be_the_quickbook_scanner_typedef<Scanner>
        {
            definition(grammar const& self) : start_rule(self.start_rule) {}
            cl::rule<scanner> const& start() const { return start_rule; }
            cl::rule<scanner> const& start_rule;
        };

        cl::rule<scanner> const& start_rule;
    };

    struct quickbook_grammar
    {
    public:
        struct impl;

    private:
        boost::scoped_ptr<impl> impl_;

    public:
        grammar command_line_macro;
        grammar inline_phrase;
        grammar phrase_start;
        grammar block_start;
        grammar doc_info;

        quickbook_grammar(quickbook::state&);
        ~quickbook_grammar();
    };
}

#endif
