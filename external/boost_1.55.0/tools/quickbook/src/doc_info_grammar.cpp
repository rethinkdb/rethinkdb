/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <map>
#include <boost/foreach.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_actor.hpp>
#include <boost/spirit/include/classic_loops.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/spirit/include/classic_chset.hpp>
#include <boost/spirit/include/classic_numerics.hpp>
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_operators.hpp>
#include "grammar_impl.hpp"
#include "state.hpp"
#include "actions.hpp"
#include "doc_info_tags.hpp"
#include "phrase_tags.hpp"

namespace quickbook
{
    namespace cl = boost::spirit::classic;

    struct attribute_info
    {
        attribute_info(value::tag_type t, cl::rule<scanner>* r)
            : tag(t), rule(r)
        {}

        value::tag_type tag;
        cl::rule<scanner>* rule;
    };

    struct doc_info_grammar_local
    {
        struct assign_attribute_type
        {
            assign_attribute_type(doc_info_grammar_local& l)
                : l(l)
            {}

            void operator()(value::tag_type& t) const {
                l.attribute_rule = *l.attribute_rules[t];
                l.attribute_tag = t;
            }
            
            doc_info_grammar_local& l;
        };
        
        struct fallback_attribute_type
        {
            fallback_attribute_type(doc_info_grammar_local& l)
                : l(l)
            {}

            void operator()(parse_iterator, parse_iterator) const {
                l.attribute_rule = l.doc_fallback;
                l.attribute_tag = value::default_tag;
            }
            
            doc_info_grammar_local& l;
        };

        cl::rule<scanner>
                        doc_info_block, doc_attribute, doc_info_attribute,
                        doc_info_escaped_attributes,
                        doc_title, doc_simple, doc_phrase, doc_fallback,
                        doc_authors, doc_author,
                        doc_copyright, doc_copyright_holder,
                        doc_source_mode, doc_biblioid, doc_compatibility_mode,
                        quickbook_version, macro, char_;
        cl::uint_parser<int, 10, 4, 4> doc_copyright_year;
        cl::symbols<> doc_types;
        cl::symbols<value::tag_type> doc_info_attributes;
        cl::symbols<value::tag_type> doc_attributes;
        std::map<value::tag_type, cl::rule<scanner>* > attribute_rules;
        value::tag_type attribute_tag;
        cl::rule<scanner> attribute_rule;
        assign_attribute_type assign_attribute;
        fallback_attribute_type fallback_attribute;

        doc_info_grammar_local()
            : assign_attribute(*this)
            , fallback_attribute(*this)
        {}

        bool source_mode_unset;
    };

    void quickbook_grammar::impl::init_doc_info()
    {
        doc_info_grammar_local& local = cleanup_.add(
            new doc_info_grammar_local);

        typedef cl::uint_parser<int, 10, 1, 2>  uint2_t;

        local.doc_types =
            "book", "article", "library", "chapter", "part"
          , "appendix", "preface", "qandadiv", "qandaset"
          , "reference", "set"
        ;

        BOOST_FOREACH(value::tag_type t, doc_attributes::tags()) {
            local.doc_attributes.add(doc_attributes::name(t), t);
            local.doc_info_attributes.add(doc_attributes::name(t), t);
        }

        BOOST_FOREACH(value::tag_type t, doc_info_attributes::tags()) {
            local.doc_info_attributes.add(doc_info_attributes::name(t), t);
        }

        // Actions
        error_action error(state);
        plain_char_action plain_char(state);
        do_macro_action do_macro(state);
        scoped_parser<to_value_scoped_action> to_value(state);
        
        doc_info_details =
                cl::eps_p                   [ph::var(local.source_mode_unset) = true]
            >>  *(  space
                >>  local.doc_attribute
                )
            >>  !(  space
                >>  local.doc_info_block
                )
            >>  *eol
            ;

        local.doc_info_block =
                '['
            >>  space
            >>  (local.doc_types >> cl::eps_p)
                                            [state.values.entry(ph::arg1, ph::arg2, doc_info_tags::type)]
            >>  hard_space
            >>  to_value(doc_info_tags::title)
                [  *(   ~cl::eps_p(blank >> (cl::ch_p('[') | ']' | cl::eol_p))
                    >>  local.char_
                    )
                    // Include 'blank' here so that it will be included in
                    // id generation.
                    >> blank
                ]
            >>  space
            >>  !(qbk_ver(106u) >> cl::eps_p(ph::var(local.source_mode_unset))
                                            [cl::assign_a(state.source_mode, "c++")]
                )
            >>  (   *(  (  local.doc_info_attribute
                        |  local.doc_info_escaped_attributes
                        )
                        >>  space
                    )
                )                           [state.values.sort()]
            >>  (   ']'
                >>  (eol | cl::end_p)
                |   cl::eps_p               [error]
                )
            ;

        local.doc_attribute =
                '['
            >>  space
            >>  local.doc_attributes        [local.assign_attribute]
            >>  hard_space
            >>  state.values.list(ph::var(local.attribute_tag))
                [   cl::eps_p               [state.values.entry(ph::arg1, ph::arg2, doc_info_tags::before_docinfo)]
                >>  local.attribute_rule
                ]
            >>  space
            >>  ']'
            ;

        local.doc_info_attribute =
                '['
            >>  space
            >>  (   local.doc_info_attributes
                                            [local.assign_attribute]
                |   (+(cl::alnum_p | '_' | '-'))
                                            [local.fallback_attribute]
                                            [error("Unrecognized document attribute: '%s'.")]
                )
            >>  hard_space
            >>  state.values.list(ph::var(local.attribute_tag))
                [local.attribute_rule]
            >>  space
            >>  ']'
            ;

        local.doc_fallback = to_value() [
            *(~cl::eps_p(']') >> local.char_)
        ];

        local.doc_info_escaped_attributes =
                ("'''" >> !eol)
            >>  (*(cl::anychar_p - "'''"))      [state.values.entry(ph::arg1, ph::arg2, doc_info_tags::escaped_attribute)]
            >>  (   cl::str_p("'''")
                |   cl::eps_p                   [error("Unclosed boostbook escape.")]
                )
                ;

        // Document Attributes

        local.quickbook_version =
                cl::uint_p                  [state.values.entry(ph::arg1)]
            >>  '.' 
            >>  uint2_t()                   [state.values.entry(ph::arg1)]
            ;

        local.attribute_rules[doc_attributes::qbk_version] = &local.quickbook_version;

        local.doc_compatibility_mode =
                cl::uint_p                  [state.values.entry(ph::arg1)]
            >>  '.'
            >>  uint2_t()                   [state.values.entry(ph::arg1)]
            ;

        local.attribute_rules[doc_attributes::compatibility_mode] = &local.doc_compatibility_mode;

        local.doc_source_mode =
                (
                   cl::str_p("c++")
                |  "python"
                |  "teletype"
                )                           [cl::assign_a(state.source_mode)]
                                            [ph::var(local.source_mode_unset) = false]
            ;

        local.attribute_rules[doc_attributes::source_mode] = &local.doc_source_mode;

        // Document Info Attributes

        local.doc_simple = to_value() [*(~cl::eps_p(']') >> local.char_)];
        local.attribute_rules[doc_info_attributes::version] = &local.doc_simple;
        local.attribute_rules[doc_info_attributes::id] = &local.doc_simple;
        local.attribute_rules[doc_info_attributes::dirname] = &local.doc_simple;
        local.attribute_rules[doc_info_attributes::category] = &local.doc_simple;
        local.attribute_rules[doc_info_attributes::last_revision] = &local.doc_simple;
        local.attribute_rules[doc_info_attributes::lang] = &local.doc_simple;
        local.attribute_rules[doc_info_attributes::xmlbase] = &local.doc_simple;

        local.doc_copyright_holder
            =   *(  ~cl::eps_p
                    (   ']'
                    |   ',' >> space >> local.doc_copyright_year
                    )
                >>  local.char_
                );

        local.doc_copyright =
            *(  +(  local.doc_copyright_year
                                            [state.values.entry(ph::arg1, doc_info_tags::copyright_year)]
                >>  space
                >>  !(  '-'
                    >>  space
                    >>  local.doc_copyright_year
                                            [state.values.entry(ph::arg1, doc_info_tags::copyright_year_end)]
                    >>  space
                    )
                >>  !cl::ch_p(',')
                >>  space
                )
            >>  to_value(doc_info_tags::copyright_name) [ local.doc_copyright_holder ]
            >>  !cl::ch_p(',')
            >>  space
            )
            ;

        local.attribute_rules[doc_info_attributes::copyright] = &local.doc_copyright;

        local.doc_phrase = to_value() [ nested_phrase ];
        local.attribute_rules[doc_info_attributes::purpose] = &local.doc_phrase;
        local.attribute_rules[doc_info_attributes::license] = &local.doc_phrase;

        local.doc_author =
                '['
            >>   space
            >>  to_value(doc_info_tags::author_surname)
                [*(~cl::eps_p(',') >> local.char_)]
            >>  ',' >> space
            >>  to_value(doc_info_tags::author_first)
                [*(~cl::eps_p(']') >> local.char_)]
            >>  ']'
            ;

        local.doc_authors =
            *(  local.doc_author
            >>  space
            >>  !(cl::ch_p(',') >> space)
            )
            ;

        local.attribute_rules[doc_info_attributes::authors] = &local.doc_authors;

        local.doc_biblioid =
                (+cl::alnum_p)              [state.values.entry(ph::arg1, ph::arg2, doc_info_tags::biblioid_class)]
            >>  hard_space
            >>  to_value(doc_info_tags::biblioid_value)
                [+(~cl::eps_p(']') >> local.char_)]
            ;

        local.attribute_rules[doc_info_attributes::biblioid] = &local.doc_biblioid;

        local.char_ =
                escape
            |   local.macro
            |   cl::anychar_p[plain_char];
            ;

        local.macro =
            cl::eps_p
            (   (   state.macro
                >>  ~cl::eps_p(cl::alpha_p | '_')
                                                // must not be followed by alpha or underscore
                )
                &   macro_identifier            // must be a valid macro for the current version
            )
            >>  state.macro                     [do_macro]
            ;
    }
}
