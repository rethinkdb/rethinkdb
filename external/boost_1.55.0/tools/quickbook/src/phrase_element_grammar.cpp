/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "grammar_impl.hpp"
#include "state.hpp"
#include "actions.hpp"
#include "utils.hpp"
#include "phrase_tags.hpp"
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/spirit/include/classic_clear_actor.hpp>
#include <boost/spirit/include/classic_if.hpp>
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_casts.hpp>

namespace quickbook
{
    namespace cl = boost::spirit::classic;

    struct phrase_element_grammar_local
    {
        cl::rule<scanner>
                        image, anchor, link, empty, cond_phrase, inner_phrase,
                        role, source_mode
                        ;
    };

    void quickbook_grammar::impl::init_phrase_elements()
    {
        phrase_element_grammar_local& local = cleanup_.add(
            new phrase_element_grammar_local);

        error_action error(state);
        raw_char_action raw_char(state);
        scoped_parser<cond_phrase_push> scoped_cond_phrase(state);
        scoped_parser<to_value_scoped_action> to_value(state);

        elements.add
            ("?", element_info(element_info::phrase, &local.cond_phrase))
            ;

        local.cond_phrase =
                blank
            >>  macro_identifier                [state.values.entry(ph::arg1, ph::arg2)]
            >>  scoped_cond_phrase() [extended_phrase]
            ;

        elements.add
            ("$", element_info(element_info::phrase, &local.image, phrase_tags::image))
            ;

        // Note that the attribute values here are encoded in plain text not
        // boostbook.
        local.image =
                qbk_ver(105u)
            >>  blank
            >>  (   qbk_ver(0, 106u)
                >>  (+(
                        *cl::space_p
                    >>  +(cl::anychar_p - (cl::space_p | phrase_end | '['))
                    ))                  [state.values.entry(ph::arg1, ph::arg2)]
                |   qbk_ver(106u)
                >>  to_value()
                    [   +(  raw_escape
                        |   (+cl::space_p >> ~cl::eps_p(phrase_end | '['))
                                        [raw_char]
                        |   (cl::anychar_p - (cl::space_p | phrase_end | '['))
                                        [raw_char]
                        )
                    ]
                )
            >>  hard_space
            >>  *state.values.list()
                [   '['
                >>  (*(cl::alnum_p | '_')) 
                                        [state.values.entry(ph::arg1, ph::arg2)]
                >>  space
                >>  (   qbk_ver(0, 106u)
                    >>  (*(cl::anychar_p - (phrase_end | '[')))
                                        [state.values.entry(ph::arg1, ph::arg2)]
                    |   qbk_ver(106u)
                    >>  to_value()
                        [   *(  raw_escape
                            |   (cl::anychar_p - (phrase_end | '['))
                                                        [raw_char]
                            )
                        ]
                    )
                >>  ']'
                >>  space
                ]
            >>  cl::eps_p(']')
            |   qbk_ver(0, 105u)
            >>  blank
            >>  (*(cl::anychar_p - phrase_end)) [state.values.entry(ph::arg1, ph::arg2)]
            >>  cl::eps_p(']')
            ;
            
        elements.add
            ("@", element_info(element_info::phrase, &local.link, phrase_tags::url))
            ("link", element_info(element_info::phrase, &local.link, phrase_tags::link))
            ("funcref", element_info(element_info::phrase, &local.link, phrase_tags::funcref))
            ("classref", element_info(element_info::phrase, &local.link, phrase_tags::classref))
            ("memberref", element_info(element_info::phrase, &local.link, phrase_tags::memberref))
            ("enumref", element_info(element_info::phrase, &local.link, phrase_tags::enumref))
            ("macroref", element_info(element_info::phrase, &local.link, phrase_tags::macroref))
            ("headerref", element_info(element_info::phrase, &local.link, phrase_tags::headerref))
            ("conceptref", element_info(element_info::phrase, &local.link, phrase_tags::conceptref))
            ("globalref", element_info(element_info::phrase, &local.link, phrase_tags::globalref))
            ;

        local.link =
                space
            >>  (   qbk_ver(0, 106u)
                >>  (*(cl::anychar_p - (']' | space)))
                                                [state.values.entry(ph::arg1, ph::arg2)]
                |   qbk_ver(106u, 107u)
                >>  to_value()
                    [   *(  raw_escape
                        |   (cl::anychar_p - (cl::ch_p('[') | ']' | space))
                                                [raw_char]
                        )
                    ]
                    >>  !(  ~cl::eps_p(comment)
                        >>  cl::eps_p('[')      [error("Open bracket in link value.")]
                        )
                |   qbk_ver(107u)
                >>  to_value() [attribute_value_1_7]
                )
            >>  hard_space
            >>  local.inner_phrase
            ;

        elements.add
            ("#", element_info(element_info::maybe_block, &local.anchor, phrase_tags::anchor))
            ;

        local.anchor =
                blank
            >>  (   qbk_ver(0, 106u)
                >>  (*(cl::anychar_p - phrase_end)) [state.values.entry(ph::arg1, ph::arg2)]
                |   qbk_ver(106u)
                >>  to_value()
                    [   *(  raw_escape
                        |   (cl::anychar_p - phrase_end)
                                                    [raw_char]
                        )
                    ]
                )
            ;

        elements.add
            ("*", element_info(element_info::phrase, &local.inner_phrase, phrase_tags::bold))
            ("'", element_info(element_info::phrase, &local.inner_phrase, phrase_tags::italic))
            ("_", element_info(element_info::phrase, &local.inner_phrase, phrase_tags::underline))
            ("^", element_info(element_info::phrase, &local.inner_phrase, phrase_tags::teletype))
            ("-", element_info(element_info::phrase, &local.inner_phrase, phrase_tags::strikethrough))
            ("\"", element_info(element_info::phrase, &local.inner_phrase, phrase_tags::quote))
            ("~", element_info(element_info::phrase, &local.inner_phrase, phrase_tags::replaceable))
            ("footnote", element_info(element_info::phrase, &local.inner_phrase, phrase_tags::footnote))
            ;

        elements.add("!", element_info(element_info::maybe_block, &local.source_mode, code_tags::next_source_mode, 107u))
            ;

        local.source_mode =
            (   cl::str_p("c++")
            |   "python"
            |   "teletype"
            )                                       [state.values.entry(ph::arg1, ph::arg2)];

        elements.add
            ("c++", element_info(element_info::phrase, &local.empty, source_mode_tags::cpp))
            ("python", element_info(element_info::phrase, &local.empty, source_mode_tags::python))
            ("teletype", element_info(element_info::phrase, &local.empty, source_mode_tags::teletype))
            ;

        elements.add
            ("role", element_info(element_info::phrase, &local.role, phrase_tags::role, 106u))
            ;

        local.role
            =   space
            >>  (+(cl::alnum_p | '_'))              [state.values.entry(ph::arg1, ph::arg2)]
            >>  hard_space
            >>  local.inner_phrase
            ;

        local.empty = cl::eps_p;

        local.inner_phrase =
                blank
            >>  to_value() [ paragraph_phrase ]
            ;
    }
}
