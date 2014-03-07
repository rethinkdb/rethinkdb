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
#include "template_tags.hpp"
#include "block_tags.hpp"
#include "phrase_tags.hpp"
#include "parsers.hpp"
#include "scoped.hpp"
#include "input_path.hpp"
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_chset.hpp>
#include <boost/spirit/include/classic_if.hpp>
#include <boost/spirit/include/classic_loops.hpp>
#include <boost/spirit/include/classic_attribute.hpp>
#include <boost/spirit/include/classic_lazy.hpp>
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/range/algorithm/find_first_of.hpp>
#include <boost/range/as_literal.hpp>

namespace quickbook
{
    namespace cl = boost::spirit::classic;

    struct list_stack_item {
        // Is this the root of the context
        // (e.g. top, template, table cell etc.)
        enum list_item_type {
            syntactic_list,   // In a list marked up '*' or '#'
            top_level,        // At the top level of a parse
                              // (might be a template body)
            nested_block      // Nested in a block element.
        } type;

        unsigned int indent;  // Indent of list marker
                              // (or paragraph if not in a list)
        unsigned int indent2; // Indent of paragraph
        char mark;            // List mark, '\0' if not in a list.

        // Example of inside a list:
        //
        //   |indent
        //   * List item
        //     |indent2 

        list_stack_item(list_item_type r) :
            type(r), indent(0), indent2(0), mark('\0') {}

        list_stack_item(char mark, unsigned int indent, unsigned int indent2) :
            type(syntactic_list), indent(indent), indent2(indent2), mark(mark)
        {}

    };

    struct block_types {
        enum values {
            none, code, list, paragraph
        };
    };

    template <typename T>
    struct member_action
    {
        typedef void(T::*member_function)(parse_iterator, parse_iterator);

        T& l;
        member_function mf;

        member_action(T& l, member_function mf) : l(l), mf(mf) {}

        void operator()(parse_iterator first, parse_iterator last) const {
            (l.*mf)(first, last);
        }
    };

    struct main_grammar_local
    {
        ////////////////////////////////////////////////////////////////////////
        // Local actions

        void start_blocks_impl(parse_iterator first, parse_iterator last);
        void start_nested_blocks_impl(parse_iterator first, parse_iterator last);
        void end_blocks_impl(parse_iterator first, parse_iterator last);
        void check_indentation_impl(parse_iterator first, parse_iterator last);
        void check_code_block_impl(parse_iterator first, parse_iterator last);
        void plain_block(string_iterator first, string_iterator last);
        void list_block(string_iterator first, string_iterator mark_pos,
                string_iterator last);
        void clear_stack();

        ////////////////////////////////////////////////////////////////////////
        // Local members

        cl::rule<scanner>
                        template_phrase, top_level, indent_check,
                        paragraph_separator, inside_paragraph,
                        code, code_line, blank_line, hr,
                        inline_code, skip_inline_code,
                        template_,
                        code_block, skip_code_block, macro,
                        template_args,
                        template_args_1_4, template_arg_1_4,
                        template_inner_arg_1_4, brackets_1_4,
                        template_args_1_5, template_arg_1_5, template_arg_1_5_content,
                        template_inner_arg_1_5, brackets_1_5,
                        template_args_1_6, template_arg_1_6, template_arg_1_6_content,
                        break_,
                        command_line_macro_identifier,
                        dummy_block, line_dummy_block, square_brackets,
                        skip_escape
                        ;

        struct block_context_closure : cl::closure<block_context_closure,
            element_info::context>
        {
            member1 is_block;
        };

        cl::rule<scanner> simple_markup, simple_markup_end;

        cl::rule<scanner> paragraph;
        cl::rule<scanner> list;
        cl::rule<scanner, block_context_closure::context_t> syntactic_block_item;
        cl::rule<scanner> common;
        cl::rule<scanner> element;

        // state
        std::stack<list_stack_item> list_stack;
        unsigned int list_indent;
        bool no_eols;
        element_info::context context;
        char mark; // Simple markup's deliminator
        bool still_in_block; // Inside a syntatic block

        // transitory state
        block_types::values block_type;
        element_info info;
        element_info::type_enum element_type;

        // state
        quickbook::state& state_;

        ////////////////////////////////////////////////////////////////////////
        // Local constructor

        main_grammar_local(quickbook::state& state)
            : list_stack()
            , list_indent(0)
            , no_eols(true)
            , context(element_info::in_top_level)
            , mark('\0')
            , state_(state)
            {}
    };

    struct process_element_impl : scoped_action_base {
        process_element_impl(main_grammar_local& l)
            : l(l), element_context_error_(false) {}

        bool start()
        {
            // This element doesn't exist in the current language version.
            if (qbk_version_n < l.info.qbk_version)
                return false;

            // The element is not allowed in this context.
            if (!(l.info.type & l.context))
            {
                if (qbk_version_n < 107u) {
                    return false;
                }
                else {
                    element_context_error_ = true;
                }
            }

            info_ = l.info;

            if (info_.type != element_info::phrase &&
                    info_.type != element_info::maybe_block)
            {
                paragraph_action para(l.state_);
                para();
            }

            assert(l.state_.values.builder.empty());

            if (!l.state_.source_mode_next.empty() &&
                info_.type != element_info::maybe_block)
            {
                l.state_.source_mode.swap(saved_source_mode_);
                l.state_.source_mode = detail::to_s(l.state_.source_mode_next.get_quickbook());
                l.state_.source_mode_next = value();
            }

            return true;
        }

        template <typename ResultT, typename ScannerT>
        bool result(ResultT result, ScannerT const& scan)
        {
            if (element_context_error_) {
                error_message_action error(l.state_,
                        "Element not allowed in this context.");
                error(scan.first, scan.first);
                return true;
            }
            else if (result) {
                return true;
            }
            else if (qbk_version_n < 107u &&
                    info_.type & element_info::in_phrase) {
                // Old versions of quickbook had a soft fail
                // for unparsed phrase elements.
                return false;
            }
            else {
                // Parse error in body.
                error_action error(l.state_);
                error(scan.first, scan.first);
                return true;
            }
        }

        void success(parse_iterator, parse_iterator) { l.element_type = info_.type; }
        void failure() { l.element_type = element_info::nothing; }

        void cleanup() {
            if (!saved_source_mode_.empty())
                l.state_.source_mode.swap(saved_source_mode_);
        }

        main_grammar_local& l;
        element_info info_;
        std::string saved_source_mode_;
        bool element_context_error_;
    };

    struct in_list_impl {
        main_grammar_local& l;

        in_list_impl(main_grammar_local& l) :
            l(l) {}

        bool operator()() const {
            return !l.list_stack.empty() &&
                l.list_stack.top().type == list_stack_item::syntactic_list;
        }
    };

    template <typename T, typename M>
    struct set_scoped_value_impl : scoped_action_base
    {
        typedef M T::*member_ptr;

        set_scoped_value_impl(T& l, member_ptr ptr)
            : l(l), ptr(ptr), saved_value() {}

        bool start(M const& value) {
            saved_value = l.*ptr;
            l.*ptr = value;

            return true;
        }

        void cleanup() {
            l.*ptr = saved_value;
        }

        T& l;
        member_ptr ptr;
        M saved_value;
    };

    template <typename T, typename M>
    struct set_scoped_value : scoped_parser<set_scoped_value_impl<T, M> >
    {
        typedef set_scoped_value_impl<T, M> impl;

        set_scoped_value(T& l, typename impl::member_ptr ptr) :
            scoped_parser<impl>(impl(l, ptr)) {}
    };

    ////////////////////////////////////////////////////////////////////////////
    // Local grammar

    void quickbook_grammar::impl::init_main()
    {
        main_grammar_local& local = cleanup_.add(
            new main_grammar_local(state));

        // Global Actions
        quickbook::element_action element_action(state);
        quickbook::paragraph_action paragraph_action(state);

        phrase_end_action end_phrase(state);
        raw_char_action raw_char(state);
        plain_char_action plain_char(state);
        escape_unicode_action escape_unicode(state);

        simple_phrase_action simple_markup(state);

        break_action break_(state);
        do_macro_action do_macro(state);

        error_action error(state);
        element_id_warning_action element_id_warning(state);

        scoped_parser<to_value_scoped_action> to_value(state);

        // Local Actions
        scoped_parser<process_element_impl> process_element(local);
        in_list_impl in_list(local);

        set_scoped_value<main_grammar_local, bool> scoped_no_eols(
                local, &main_grammar_local::no_eols);
        set_scoped_value<main_grammar_local, element_info::context> scoped_context(
                local, &main_grammar_local::context);
        set_scoped_value<main_grammar_local, bool> scoped_still_in_block(
                local, &main_grammar_local::still_in_block);

        member_action<main_grammar_local> check_indentation(local,
            &main_grammar_local::check_indentation_impl);
        member_action<main_grammar_local> check_code_block(local,
            &main_grammar_local::check_code_block_impl);
        member_action<main_grammar_local> start_blocks(local,
            &main_grammar_local::start_blocks_impl);
        member_action<main_grammar_local> start_nested_blocks(local,
            &main_grammar_local::start_nested_blocks_impl);
        member_action<main_grammar_local> end_blocks(local,
            &main_grammar_local::end_blocks_impl);

        // phrase/phrase_start is used for an entirely self-contained
        // phrase. For example, any remaining anchors are written out
        // at the end instead of being saved for any following content.
        phrase_start =
            inline_phrase                       [end_phrase]
            ;

        // nested_phrase is used for a phrase nested inside square
        // brackets.
        nested_phrase =
            state.values.save()
            [
                scoped_context(element_info::in_phrase)
                [*(~cl::eps_p(']') >> local.common)]
            ]
            ;

        // paragraph_phrase is like a nested_phrase but is also terminated
        // by a paragraph end.
        paragraph_phrase =
            state.values.save()
            [
                scoped_context(element_info::in_phrase)
                [*(~cl::eps_p(phrase_end) >> local.common)]
            ]
            ;

        // extended_phrase is like a paragraph_phrase but allows some block
        // elements.
        extended_phrase =
            state.values.save()
            [
                scoped_context(element_info::in_conditional)
                [*(~cl::eps_p(phrase_end) >> local.common)]
            ]
            ;

        // inline_phrase is used a phrase that isn't nested inside
        // brackets, but is not self contained. An example of this
        // is expanding a template, which is parsed separately but
        // is part of the paragraph that contains it.
        inline_phrase =
            state.values.save()
            [   qbk_ver(107u)
            >>  local.template_phrase
            |   qbk_ver(0, 107u)
            >>  scoped_context(element_info::in_phrase)
                [*local.common]
            ]
            ;

        table_title_phrase =
            state.values.save()
            [
                scoped_context(element_info::in_phrase)
                [   *( ~cl::eps_p(space >> (']' | '[' >> space >> '['))
                    >>  local.common
                    )
                ]
            ]
            ;

        inside_preformatted =
            scoped_no_eols(false)
            [   paragraph_phrase
            ]
            ;

        // Phrase templates can contain block tags, but can't contain
        // syntatic blocks.
        local.template_phrase =
                scoped_context(element_info::in_top_level)
                [   *(  (local.paragraph_separator >> space >> cl::anychar_p)
                                        [error("Paragraph in phrase template.")]
                    |   local.common
                    )
                ]
            ;

        // Top level blocks
        block_start =
                (*eol)                  [start_blocks]
            >>  (   *(  local.top_level
                    >>  !(  qbk_ver(106u)
                        >>  cl::ch_p(']')
                        >>  cl::eps_p   [error("Mismatched close bracket")]
                        )
                    )
                )                       [end_blocks]
            ;

        // Blocks contains within an element, e.g. a table cell or a footnote.
        inside_paragraph =
            state.values.save()
            [   cl::eps_p               [start_nested_blocks]
            >>  (   qbk_ver(107u)
                >>  (*eol)
                >>  (*local.top_level)
                |   qbk_ver(0, 107u)
                >>  local.inside_paragraph
                )                       [end_blocks]
            ]
            ;

        local.top_level =
                cl::eps_p(local.indent_check)
            >>  (   cl::eps_p(ph::var(local.block_type) == block_types::code)
                >>  local.code
                |   cl::eps_p(ph::var(local.block_type) == block_types::list)
                >>  local.list
                |   cl::eps_p(ph::var(local.block_type) == block_types::paragraph)
                >>  (   local.hr
                    |   local.paragraph
                    )
                )
            >>  *eol
            ;

        local.indent_check =
            (   *cl::blank_p
            >>  !(  (cl::ch_p('*') | '#')
                >> *cl::blank_p)
            )                                   [check_indentation]
            ;

        local.paragraph =
                                                // Usually superfluous call
                                                // for paragraphs in lists.
            cl::eps_p                           [paragraph_action]
        >>  scoped_context(element_info::in_top_level)
            [   scoped_still_in_block(true)
                [   local.syntactic_block_item(element_info::is_contextual_block)
                >>  *(  cl::eps_p(ph::var(local.still_in_block))
                    >>  local.syntactic_block_item(element_info::is_block)
                    )
                ]
            ]                                   [paragraph_action]
            ;

        local.list =
                *cl::blank_p
            >>  (cl::ch_p('*') | '#')
            >>  (*cl::blank_p)
            >>  scoped_context(element_info::in_list_block)
                [   scoped_still_in_block(true)
                    [   *(  cl::eps_p(ph::var(local.still_in_block))
                        >>  local.syntactic_block_item(element_info::is_block)
                        )
                    ]
                ]
            ;

        local.syntactic_block_item =
                local.paragraph_separator       [ph::var(local.still_in_block) = false]
            |   (cl::eps_p(~cl::ch_p(']')) | qbk_ver(0, 107u))
                                                [ph::var(local.element_type) = element_info::nothing]
            >>  local.common
                // If the element is a block, then a newline will end the
                // current syntactic block.
                // Note that we don't do this for lists in 1.6 to avoid messing
                // up on nested block elements.
            >>  !(  cl::eps_p(in_list) >> qbk_ver(106u)
                |   cl::eps_p
                    (
                        ph::static_cast_<int>(local.syntactic_block_item.is_block) &
                        ph::static_cast_<int>(ph::var(local.element_type))
                    )
                >>  eol                         [ph::var(local.still_in_block) = false]
                )
            ;

        local.paragraph_separator =
                cl::eol_p
            >>  cl::eps_p
                (   *cl::blank_p
                >>  (   cl::eol_p
                    |   cl::end_p
                    |   cl::eps_p(in_list) >> (cl::ch_p('*') | '#')
                    )
                )
            >>  *eol
            ;

        // Blocks contains within an element, e.g. a table cell or a footnote.
        local.inside_paragraph =
            scoped_context(element_info::in_nested_block)
            [   *(  local.paragraph_separator   [paragraph_action]
                |   ~cl::eps_p(']')
                >>  local.common
                )
            ]                                   [paragraph_action]
            ;

        local.hr =
                cl::str_p("----")
            >>  state.values.list(block_tags::hr)
                [   (   qbk_ver(106u)
                    >>  *(line_comment | (cl::anychar_p - (cl::eol_p | '[' | ']')))
                    |   qbk_ver(0, 106u)
                    >>  *(line_comment | (cl::anychar_p - (cl::eol_p | "[/")))
                    )
                >>  *eol
                ]                               [element_action]
            ;

        local.element
            =   '['
            >>  (   cl::eps_p(cl::punct_p)
                >>  elements                    [ph::var(local.info) = ph::arg1]
                |   elements                    [ph::var(local.info) = ph::arg1]
                >>  (cl::eps_p - (cl::alnum_p | '_'))
                )
            >>  process_element()
                [   state.values.list(ph::var(local.info.tag))
                    [   cl::lazy_p(*ph::var(local.info.rule))
                    >>  space
                    >>  ']'
                    ]                           [element_action]
                ]
            ;

        local.code =
            state.values.list(code_tags::code_block)
            [(  local.code_line
                >> *(*local.blank_line >> local.code_line)
            )                                   [state.values.entry(ph::arg1, ph::arg2)]
            ]                                   [element_action]
            >> *eol
            ;

        local.code_line =
            (   *cl::blank_p
            >>  ~cl::eps_p(cl::eol_p)
            )                                   [check_code_block]
        >>  cl::eps_p(ph::var(local.block_type) == block_types::code)
        >>  *(cl::anychar_p - cl::eol_p)
        >>  (cl::eol_p | cl::end_p)
            ;

        local.blank_line =
            *cl::blank_p >> cl::eol_p
            ;

        local.common =
                local.macro
            |   local.element
            |   local.template_
            |   local.break_
            |   local.code_block
            |   local.inline_code
            |   local.simple_markup
            |   escape
            |   comment
            |   qbk_ver(106u) >> local.square_brackets
            |   cl::space_p                 [raw_char]
            |   cl::anychar_p               [plain_char]
            ;

        skip_entity =
                '['
                // For escaped templates:
            >>  !(space >> cl::ch_p('`') >> (cl::alpha_p | '_'))
            >>  *(~cl::eps_p(']') >> skip_entity)
            >>  !cl::ch_p(']')
            |   local.skip_code_block
            |   local.skip_inline_code
            |   local.skip_escape
            |   comment
            |   (cl::anychar_p - '[' - ']')
            ;

        local.square_brackets =
            (   cl::ch_p('[')           [plain_char]
            >>  paragraph_phrase
            >>  (   cl::ch_p(']')       [plain_char]
                |   cl::eps_p           [error("Missing close bracket")]
                )
            |   cl::ch_p(']')           [plain_char]
            >>  cl::eps_p               [error("Mismatched close bracket")]
            )
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

        local.template_ =
            (   '['
            >>  space
            >>  state.values.list(template_tags::template_)
                [   (   cl::str_p('`')
                    >>  cl::eps_p(cl::punct_p)
                    >>  state.templates.scope
                            [state.values.entry(ph::arg1, ph::arg2, template_tags::escape)]
                            [state.values.entry(ph::arg1, ph::arg2, template_tags::identifier)]
                    >>  !qbk_ver(106u)
                            [error("Templates with punctuation names can't be escaped in quickbook 1.6+")]
                    |   cl::str_p('`')
                    >>  state.templates.scope
                            [state.values.entry(ph::arg1, ph::arg2, template_tags::escape)]
                            [state.values.entry(ph::arg1, ph::arg2, template_tags::identifier)]

                    |   cl::eps_p(cl::punct_p)
                    >>  state.templates.scope
                            [state.values.entry(ph::arg1, ph::arg2, template_tags::identifier)]

                    |   state.templates.scope
                            [state.values.entry(ph::arg1, ph::arg2, template_tags::identifier)]
                    >>  cl::eps_p(hard_space)
                    )
                >>  space
                >>  !local.template_args
                >>  ']'
                ]
            )                                   [element_action]
            ;

        local.template_args =
                qbk_ver(106u) >> local.template_args_1_6
            |   qbk_ver(105u, 106u) >> local.template_args_1_5
            |   qbk_ver(0, 105u) >> local.template_args_1_4
            ;

        local.template_args_1_4 = local.template_arg_1_4 >> *(".." >> local.template_arg_1_4);

        local.template_arg_1_4 =
            (   cl::eps_p(*cl::blank_p >> cl::eol_p)
            >>  local.template_inner_arg_1_4    [state.values.entry(ph::arg1, ph::arg2, template_tags::block)]
            |   local.template_inner_arg_1_4    [state.values.entry(ph::arg1, ph::arg2, template_tags::phrase)]
            )                               
            ;

        local.template_inner_arg_1_4 =
            +(local.brackets_1_4 | (cl::anychar_p - (cl::str_p("..") | ']')))
            ;

        local.brackets_1_4 =
            '[' >> local.template_inner_arg_1_4 >> ']'
            ;

        local.template_args_1_5 = local.template_arg_1_5 >> *(".." >> local.template_arg_1_5);

        local.template_arg_1_5 =
            (   cl::eps_p(*cl::blank_p >> cl::eol_p)
            >>  local.template_arg_1_5_content  [state.values.entry(ph::arg1, ph::arg2, template_tags::block)]
            |   local.template_arg_1_5_content  [state.values.entry(ph::arg1, ph::arg2, template_tags::phrase)]
            )
            ;

        local.template_arg_1_5_content =
            +(local.brackets_1_5 | ('\\' >> cl::anychar_p) | (cl::anychar_p - (cl::str_p("..") | '[' | ']')))
            ;

        local.template_inner_arg_1_5 =
            +(local.brackets_1_5 | ('\\' >> cl::anychar_p) | (cl::anychar_p - (cl::str_p('[') | ']')))
            ;

        local.brackets_1_5 =
            '[' >> local.template_inner_arg_1_5 >> ']'
            ;

        local.template_args_1_6 = local.template_arg_1_6 >> *(".." >> local.template_arg_1_6);

        local.template_arg_1_6 =
            (   cl::eps_p(*cl::blank_p >> cl::eol_p)
            >>  local.template_arg_1_6_content  [state.values.entry(ph::arg1, ph::arg2, template_tags::block)]
            |   local.template_arg_1_6_content  [state.values.entry(ph::arg1, ph::arg2, template_tags::phrase)]
            )
            ;

        local.template_arg_1_6_content =
            + ( ~cl::eps_p("..") >> skip_entity )
            ;

        local.break_
            =   (   '['
                >>  space
                >>  "br"
                >>  space
                >>  ']'
                )                               [break_]
                ;

        local.inline_code =
            '`' >> state.values.list(code_tags::inline_code)
            [(
               *(cl::anychar_p -
                    (   '`'
                    |   (cl::eol_p >> *cl::blank_p >> cl::eol_p)
                                                // Make sure that we don't go
                    )                           // past a single block
                ) >> cl::eps_p('`')
            )                                   [state.values.entry(ph::arg1, ph::arg2)]
            >>  '`'
            ]                                   [element_action]
            ;

        local.skip_inline_code =
                '`'
            >>  *(cl::anychar_p -
                    (   '`'
                    |   (cl::eol_p >> *cl::blank_p >> cl::eol_p)
                                                // Make sure that we don't go
                    )                           // past a single block
                )
            >>  !cl::ch_p('`')
            ;

        local.skip_code_block =
                "```"
            >>  ~cl::eps_p("`")
            >>  (   !(  *(*cl::blank_p >> cl::eol_p)
                    >>  (   *(  "````" >> *cl::ch_p('`')
                            |   (   cl::anychar_p
                                -   (*cl::space_p >> "```" >> ~cl::eps_p("`"))
                                )
                            )
                            >>  !(*cl::blank_p >> cl::eol_p)
                        )
                    >>  (*cl::space_p >> "```")
                    )
                |   *cl::anychar_p
                )
            |   "``"
            >>  ~cl::eps_p("`")
            >>  (   (   *(*cl::blank_p >> cl::eol_p)
                    >>  (   *(  "```" >> *cl::ch_p('`')
                            |   (   cl::anychar_p
                                -   (*cl::space_p >> "``" >> ~cl::eps_p("`"))
                                )
                            )
                            >>  !(*cl::blank_p >> cl::eol_p)
                        )
                    >>  (*cl::space_p >> "``")
                    )
                |   *cl::anychar_p
                )
            ;

        local.code_block =
                "```"
            >>  ~cl::eps_p("`")
            >>  (   state.values.list(code_tags::inline_code_block)
                    [   *(*cl::blank_p >> cl::eol_p)
                    >>  (   *(  "````" >> *cl::ch_p('`')
                            |   (   cl::anychar_p
                                -   (*cl::space_p >> "```" >> ~cl::eps_p("`"))
                                )
                            )
                            >>  !(*cl::blank_p >> cl::eol_p)
                        )                   [state.values.entry(ph::arg1, ph::arg2)]
                    >>  (*cl::space_p >> "```")
                    ]                       [element_action]
                |   cl::eps_p               [error("Unfinished code block")]
                >>  *cl::anychar_p
                )
            |   "``"
            >>  ~cl::eps_p("`")
            >>  (   state.values.list(code_tags::inline_code_block)
                    [   *(*cl::blank_p >> cl::eol_p)
                    >>  (   *(  "```" >> *cl::ch_p('`')
                            |   (   cl::anychar_p
                                -   (*cl::space_p >> "``" >> ~cl::eps_p("`"))
                                )
                            )
                            >>  !(*cl::blank_p >> cl::eol_p)
                        )                   [state.values.entry(ph::arg1, ph::arg2)]
                    >>  (*cl::space_p >> "``")
                    ]                       [element_action]
                |   cl::eps_p               [error("Unfinished code block")]
                >>  *cl::anychar_p
                )
            ;

        local.simple_markup =
                cl::chset<>("*/_=")             [ph::var(local.mark) = ph::arg1]
            >>  cl::eps_p(cl::graph_p)          // graph_p must follow first mark
            >>  lookback
                [   cl::anychar_p               // skip back over the markup
                >>  ~cl::eps_p(cl::ch_p(boost::ref(local.mark)))
                                                // first mark not be preceeded by
                                                // the same character.
                >>  (cl::space_p | cl::punct_p | cl::end_p)
                                                // first mark must be preceeded
                                                // by space or punctuation or the
                                                // mark character or a the start.
                ]
            >>  state.values.save()
                [
                    to_value()
                    [
                        cl::eps_p((state.macro & macro_identifier) >> local.simple_markup_end)
                    >>  state.macro       [do_macro]
                    |   ~cl::eps_p(cl::ch_p(boost::ref(local.mark)))
                    >>  +(  ~cl::eps_p
                            (   lookback [~cl::ch_p(boost::ref(local.mark))]
                            >>  local.simple_markup_end
                            )
                        >>  cl::anychar_p   [plain_char]
                        )
                    ]
                >>  cl::ch_p(boost::ref(local.mark))
                                                [simple_markup]
                ]
            ;

        local.simple_markup_end
            =   (   lookback[cl::graph_p]       // final mark must be preceeded by
                                                // graph_p
                >>  cl::ch_p(boost::ref(local.mark))
                >>  ~cl::eps_p(cl::ch_p(boost::ref(local.mark)))
                                                // final mark not be followed by
                                                // the same character.
                >>  (cl::space_p | cl::punct_p | cl::end_p)
                                                 // final mark must be followed by
                                                 // space or punctuation
                )
            |   '['
            |   "'''"
            |   '`'
            |   phrase_end
                ;

        escape =
                cl::str_p("\\n")                [break_]
            |   cl::str_p("\\ ")                // ignore an escaped space
            |   '\\' >> cl::punct_p             [plain_char]
            |   "\\u" >> cl::repeat_p(4) [cl::chset<>("0-9a-fA-F")]
                                                [escape_unicode]
            |   "\\U" >> cl::repeat_p(8) [cl::chset<>("0-9a-fA-F")]
                                                [escape_unicode]
            |   ("'''" >> !eol)
            >>  state.values.save()
                [   (*(cl::anychar_p - "'''"))  [state.values.entry(ph::arg1, ph::arg2, phrase_tags::escape)]
                >>  (   cl::str_p("'''")
                    |   cl::eps_p               [error("Unclosed boostbook escape.")]
                    )                           [element_action]
                ]
            ;

        local.skip_escape =
                cl::str_p("\\n")
            |   cl::str_p("\\ ")
            |   '\\' >> cl::punct_p
            |   "\\u" >> cl::repeat_p(4) [cl::chset<>("0-9a-fA-F")]
            |   "\\U" >> cl::repeat_p(8) [cl::chset<>("0-9a-fA-F")]
            |   ("'''" >> !eol)
            >>  (*(cl::anychar_p - "'''"))
            >>  (   cl::str_p("'''")
                |   cl::eps_p
                )
            ;

        raw_escape =
                cl::str_p("\\n")                [error("Newlines invalid here.")]
            |   cl::str_p("\\ ")                // ignore an escaped space
            |   '\\' >> cl::punct_p             [raw_char]
            |   "\\u" >> cl::repeat_p(4) [cl::chset<>("0-9a-fA-F")]
                                                [escape_unicode]
            |   "\\U" >> cl::repeat_p(8) [cl::chset<>("0-9a-fA-F")]
                                                [escape_unicode]
            |   ('\\' >> cl::anychar_p)         [error("Invalid escape.")]
                                                [raw_char]
            |   ("'''" >> !eol)                 [error("Boostbook escape invalid here.")]
            >>  (*(cl::anychar_p - "'''"))
            >>  (   cl::str_p("'''")
                |   cl::eps_p                   [error("Unclosed boostbook escape.")]
                )
            ;

        attribute_value_1_7 =
            *(  ~cl::eps_p(']' | cl::space_p | comment)
            >>  (   cl::eps_p
                    (   cl::ch_p('[')
                    >>  space
                    >>  (   cl::eps_p(cl::punct_p)
                        >>  elements
                        |   elements
                        >>  (cl::eps_p - (cl::alnum_p | '_'))
                        )
                    )                           [error("Elements not allowed in attribute values.")]
                >>  local.square_brackets
                |   local.template_
                |   cl::eps_p(cl::ch_p('['))    [error("Unmatched template in attribute value.")]
                >>  local.square_brackets
                |   raw_escape
                |   cl::anychar_p               [raw_char]
                )
            )
            ;

        //
        // Command line
        //

        command_line =
            state.values.list(block_tags::macro_definition)
            [   *cl::space_p
            >>  local.command_line_macro_identifier
                                                [state.values.entry(ph::arg1, ph::arg2)]
            >>  *cl::space_p
            >>  !(   '='
                >>  *cl::space_p
                >>  to_value() [ inline_phrase ]
                >>  *cl::space_p
                )
            >>  cl::end_p
            ]                                   [element_action]
            ;

        local.command_line_macro_identifier =
                qbk_ver(106u)
            >>  +(cl::anychar_p - (cl::space_p | '[' | '\\' | ']' | '='))
            |   +(cl::anychar_p - (cl::space_p | ']' | '='))
            ;

        // Miscellaneous stuff

        // Follows an alphanumeric identifier - ensures that it doesn't
        // match an empty space in the middle of the identifier.
        hard_space =
            (cl::eps_p - (cl::alnum_p | '_')) >> space
            ;

        space =
            *(cl::space_p | comment)
            ;

        blank =
            *(cl::blank_p | comment)
            ;

        eol = blank >> cl::eol_p
            ;

        phrase_end =
                ']'
            |   cl::eps_p(ph::var(local.no_eols))
            >>  cl::eol_p >> *cl::blank_p >> cl::eol_p
            ;                                   // Make sure that we don't go
                                                // past a single block, except
                                                // when preformatted.

        comment =
            "[/" >> *(local.dummy_block | (cl::anychar_p - ']')) >> ']'
            ;

        local.dummy_block =
            '[' >> *(local.dummy_block | (cl::anychar_p - ']')) >> ']'
            ;

        line_comment =
            "[/" >> *(local.line_dummy_block | (cl::anychar_p - (cl::eol_p | ']'))) >> ']'
            ;

        local.line_dummy_block =
            '[' >> *(local.line_dummy_block | (cl::anychar_p - (cl::eol_p | ']'))) >> ']'
            ;

        macro_identifier =
                qbk_ver(106u)
            >>  +(cl::anychar_p - (cl::space_p | '[' | '\\' | ']'))
            |   qbk_ver(0, 106u)
            >>  +(cl::anychar_p - (cl::space_p | ']'))
            ;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Indentation Handling

    template <typename Iterator>
    int indent_length(Iterator first, Iterator end)
    {
        int length = 0;
        for(; first != end; ++first)
        {
            if (*first == '\t') {
                // hardcoded tab to 4 for now
                length = length + 4 - (length % 4);
            }
            else {
                ++length;
            }
        }

        return length;
    }

    void main_grammar_local::start_blocks_impl(parse_iterator, parse_iterator)
    {
        list_stack.push(list_stack_item(list_stack_item::top_level));
    }

    void main_grammar_local::start_nested_blocks_impl(parse_iterator, parse_iterator)
    {
        // If this nested block is part of a list, then tell the
        // output state.
        //
        // TODO: This is a bit dodgy, it would be better if this
        // was handled when the output state is pushed (currently
        // in to_value_scoped_action).
        state_.in_list = state_.explicit_list;
        state_.explicit_list = false;

        list_stack.push(list_stack_item(list_stack_item::nested_block));
    }

    void main_grammar_local::end_blocks_impl(parse_iterator, parse_iterator)
    {
        clear_stack();
        list_stack.pop();
    }

    void main_grammar_local::check_indentation_impl(parse_iterator first_, parse_iterator last_)
    {
        string_iterator first = first_.base();
        string_iterator last = last_.base();
        string_iterator mark_pos = boost::find_first_of(
            boost::make_iterator_range(first, last),
            boost::as_literal("#*"));

        if (mark_pos == last) {
            plain_block(first, last);
        }
        else {
            list_block(first, mark_pos, last);
        }
    }

    void main_grammar_local::check_code_block_impl(parse_iterator first, parse_iterator last)
    {
        unsigned int new_indent = indent_length(first.base(), last.base());

        block_type = (new_indent > list_stack.top().indent2) ?
             block_types::code : block_types::none;
    }

    void main_grammar_local::plain_block(string_iterator first, string_iterator last)
    {
        if (qbk_version_n >= 106u) {
            unsigned int new_indent = indent_length(first, last);

            if (new_indent > list_stack.top().indent2) {
                if (list_stack.top().type != list_stack_item::nested_block) {
                    block_type = block_types::code;
                }
                else {
                    block_type = block_types::paragraph;
                }
            }
            else {
                while (list_stack.top().type == list_stack_item::syntactic_list
                        && new_indent < list_stack.top().indent)
                {
                    state_.end_list_item();
                    state_.end_list(list_stack.top().mark);
                    list_stack.pop();
                    list_indent = list_stack.top().indent;
                }

                if (list_stack.top().type == list_stack_item::syntactic_list
                        && new_indent == list_stack.top().indent)
                {
                    // If the paragraph is aligned with the list item's marker,
                    // then end the current list item if that's aligned (or to
                    // the left of) the parent's paragraph.
                    //
                    // i.e.
                    //
                    // * Level 1
                    //    * Level 2
                    //
                    //    Still Level 2
                    //
                    // vs.
                    //
                    // * Level 1
                    //   * Level 2
                    //
                    //   Back to Level 1
                
                    list_stack_item save = list_stack.top();
                    list_stack.pop();

                    assert(list_stack.top().type != list_stack_item::syntactic_list ?
                        new_indent >= list_stack.top().indent :
                        new_indent > list_stack.top().indent);

                    if (new_indent <= list_stack.top().indent2) {
                        state_.end_list_item();
                        state_.end_list(save.mark);
                        list_indent = list_stack.top().indent;
                    }
                    else {
                        list_stack.push(save);
                    }
                }

                block_type = block_types::paragraph;
            }

            if (qbk_version_n == 106u &&
                    list_stack.top().type == list_stack_item::syntactic_list) {
                detail::outerr(state_.current_file, first)
                    << "Nested blocks in lists won't be supported in "
                    << "quickbook 1.6"
                    << std::endl;
                ++state_.error_count;
            }
        }
        else {
            clear_stack();

            if (list_stack.top().type != list_stack_item::nested_block &&
                    last != first)
                block_type = block_types::code;
            else
                block_type = block_types::paragraph;
        }
    }

    void main_grammar_local::list_block(string_iterator first, string_iterator mark_pos,
            string_iterator last)
    {
        unsigned int new_indent = indent_length(first, mark_pos);
        unsigned int new_indent2 = indent_length(first, last);
        char mark = *mark_pos;

        if (list_stack.top().type == list_stack_item::top_level &&
                new_indent > 0) {
            block_type = block_types::code;
            return;
        }

        if (list_stack.top().type != list_stack_item::syntactic_list ||
                new_indent > list_indent) {
            list_stack.push(list_stack_item(mark, new_indent, new_indent2));
            state_.start_list(mark);
        }
        else if (new_indent == list_indent) {
            state_.end_list_item();
        }
        else {
            // This should never reach root, since the first list
            // has indentation 0.
            while(list_stack.top().type == list_stack_item::syntactic_list &&
                    new_indent < list_stack.top().indent)
            {
                state_.end_list_item();
                state_.end_list(list_stack.top().mark);
                list_stack.pop();
            }

            state_.end_list_item();
        }

        list_indent = new_indent;

        if (mark != list_stack.top().mark)
        {
            detail::outerr(state_.current_file, first)
                << "Illegal change of list style.\n";
            detail::outwarn(state_.current_file, first)
                << "Ignoring change of list style." << std::endl;
            ++state_.error_count;
        }

        state_.start_list_item();
        block_type = block_types::list;
    }

    void main_grammar_local::clear_stack()
    {
        while (list_stack.top().type == list_stack_item::syntactic_list) {
            state_.end_list_item();
            state_.end_list(list_stack.top().mark);
            list_stack.pop();
        }
    }
}
