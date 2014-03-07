/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_confix.hpp>
#include <boost/spirit/include/classic_chset.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/spirit/include/classic_loops.hpp>
#include "grammar.hpp"
#include "state.hpp"
#include "actions.hpp"
#include "utils.hpp"
#include "files.hpp"
#include "input_path.hpp"

namespace quickbook
{    
    namespace cl = boost::spirit::classic;

    template <typename T, typename Value>
    struct member_action_value
    {
        typedef void(T::*member_function)(Value);

        T& l;
        member_function mf;

        member_action_value(T& l, member_function mf) : l(l), mf(mf) {}

        void operator()(Value v) const {
            (l.*mf)(v);
        }
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

    template <typename T, typename Arg1>
    struct member_action1
    {
        typedef void(T::*member_function)(parse_iterator, parse_iterator, Arg1);

        T& l;
        member_function mf;

        member_action1(T& l, member_function mf) : l(l), mf(mf) {}

        struct impl
        {
            member_action1 a;
            Arg1 value;

            impl(member_action1& a, Arg1 value) :
                a(a), value(value)
            {}

            void operator()(parse_iterator first, parse_iterator last) const {
                (a.l.*a.mf)(first, last, value);
            }
        };

        impl operator()(Arg1 a1) {
            return impl(*this, a1);
        }
    };

    // Syntax Highlight Actions

    struct syntax_highlight_actions
    {
        quickbook::state& state;
        do_macro_action do_macro_impl;

        // State
        bool support_callouts;
        boost::string_ref marked_text;

        syntax_highlight_actions(quickbook::state& state, bool is_block) :
            state(state),
            do_macro_impl(state),
            support_callouts(is_block && (qbk_version_n >= 107u ||
                state.current_file->is_code_snippets)),
            marked_text()
        {}

        void span(parse_iterator, parse_iterator, char const*);
        void span_start(parse_iterator, parse_iterator, char const*);
        void span_end(parse_iterator, parse_iterator);
        void unexpected_char(parse_iterator, parse_iterator);
        void plain_char(parse_iterator, parse_iterator);
        void pre_escape_back(parse_iterator, parse_iterator);
        void post_escape_back(parse_iterator, parse_iterator);
        void do_macro(std::string const&);

        void mark_text(parse_iterator, parse_iterator);
        void callout(parse_iterator, parse_iterator);
    };

    void syntax_highlight_actions::span(parse_iterator first,
            parse_iterator last, char const* name)
    {
        state.phrase << "<phrase role=\"" << name << "\">";
        while (first != last)
            detail::print_char(*first++, state.phrase.get());
        state.phrase << "</phrase>";
    }

    void syntax_highlight_actions::span_start(parse_iterator first,
            parse_iterator last, char const* name)
    {
        state.phrase << "<phrase role=\"" << name << "\">";
        while (first != last)
            detail::print_char(*first++, state.phrase.get());
    }

    void syntax_highlight_actions::span_end(parse_iterator first,
            parse_iterator last)
    {
        while (first != last)
            detail::print_char(*first++, state.phrase.get());
        state.phrase << "</phrase>";
    }

    void syntax_highlight_actions::unexpected_char(parse_iterator first,
            parse_iterator last)
    {
        file_position const pos = state.current_file->position_of(first.base());

        detail::outwarn(state.current_file->path, pos.line)
            << "in column:" << pos.column
            << ", unexpected character: " << std::string(first.base(), last.base())
            << "\n";

        // print out an unexpected character
        state.phrase << "<phrase role=\"error\">";
        while (first != last)
            detail::print_char(*first++, state.phrase.get());
        state.phrase << "</phrase>";
    }

    void syntax_highlight_actions::plain_char(parse_iterator first,
            parse_iterator last)
    {
        while (first != last)
            detail::print_char(*first++, state.phrase.get());
    }

    void syntax_highlight_actions::pre_escape_back(parse_iterator,
            parse_iterator)
    {
        state.push_output(); // save the stream
    }

    void syntax_highlight_actions::post_escape_back(parse_iterator,
            parse_iterator)
    {
        std::string tmp;
        state.phrase.swap(tmp);
        state.pop_output(); // restore the stream
        state.phrase << tmp;
    }

    void syntax_highlight_actions::do_macro(std::string const& v)
    {
        do_macro_impl(v);
    }

    void syntax_highlight_actions::mark_text(parse_iterator first,
            parse_iterator last)
    {
        marked_text = boost::string_ref(first.base(), last.base() - first.base());
    }

    void syntax_highlight_actions::callout(parse_iterator, parse_iterator)
    {
        state.phrase << state.add_callout(qbk_value(state.current_file,
            marked_text.begin(), marked_text.end()));
        marked_text.clear();
    }

    // Syntax

    struct keywords_holder
    {
        cl::symbols<> cpp, python;

        keywords_holder()
        {
            cpp
                    =   "alignas", "alignof", "and_eq", "and", "asm", "auto",
                        "bitand", "bitor", "bool", "break", "case", "catch",
                        "char", "char16_t", "char32_t", "class", "compl",
                        "const", "const_cast", "constexpr", "continue",
                        "decltype", "default", "delete", "do", "double",
                        "dynamic_cast",  "else", "enum", "explicit", "export",
                        "extern", "false", "float", "for", "friend", "goto",
                        "if", "inline", "int", "long", "mutable", "namespace",
                        "new", "noexcept", "not_eq", "not", "nullptr",
                        "operator", "or_eq", "or", "private", "protected",
                        "public", "register", "reinterpret_cast", "return",
                        "short", "signed", "sizeof", "static", "static_assert",
                        "static_cast", "struct", "switch", "template", "this",
                        "thread_local", "throw", "true", "try", "typedef",
                        "typeid", "typename", "union", "unsigned", "using",
                        "virtual", "void", "volatile", "wchar_t", "while",
                        "xor_eq", "xor"
                    ;

            python
                    =
                    "and",       "del",       "for",       "is",        "raise",
                    "assert",    "elif",      "from",      "lambda",    "return",
                    "break",     "else",      "global",    "not",       "try",
                    "class",     "except",    "if",        "or",        "while",
                    "continue",  "exec",      "import",    "pass",      "yield",
                    "def",       "finally",   "in",        "print",

                    // Technically "as" and "None" are not yet keywords (at Python
                    // 2.4). They are destined to become keywords, and we treat them
                    // as such for syntax highlighting purposes.

                    "as", "None"
                    ;
        }
    };

    namespace {
        keywords_holder keywords;
    }

    // Grammar for C++ highlighting
    struct cpp_highlight : public cl::grammar<cpp_highlight>
    {
        cpp_highlight(syntax_highlight_actions& actions)
            : actions(actions) {}

        template <typename Scanner>
        struct definition
        {
            definition(cpp_highlight const& self)
                : g(self.actions.state.grammar())
            {
                member_action1<syntax_highlight_actions, char const*>
                    span(self.actions, &syntax_highlight_actions::span),
                    span_start(self.actions, &syntax_highlight_actions::span_start);
                member_action<syntax_highlight_actions>
                    span_end(self.actions, &syntax_highlight_actions::span_end),
                    unexpected_char(self.actions, &syntax_highlight_actions::unexpected_char),
                    plain_char(self.actions, &syntax_highlight_actions::plain_char),
                    pre_escape_back(self.actions, &syntax_highlight_actions::pre_escape_back),
                    post_escape_back(self.actions, &syntax_highlight_actions::post_escape_back),
                    mark_text(self.actions, &syntax_highlight_actions::mark_text),
                    callout(self.actions, &syntax_highlight_actions::callout);
                member_action_value<syntax_highlight_actions, std::string const&>
                    do_macro(self.actions, &syntax_highlight_actions::do_macro);
                error_action error(self.actions.state);

                program =
                    *(  (*cl::space_p)                  [plain_char]
                    >>  (line_start | rest_of_line)
                    >>  *rest_of_line
                    )
                    ;

                line_start =
                        preprocessor                    [span("preprocessor")]
                    ;
                
                rest_of_line = 
                        (+cl::blank_p)                  [plain_char]
                    |   macro
                    |   escape
                    |   cl::eps_p(ph::var(self.actions.support_callouts))
                    >>  (   line_callout                [callout]
                        |   inline_callout              [callout]
                        )
                    |   comment
                    |   keyword                         [span("keyword")]
                    |   identifier                      [span("identifier")]
                    |   special                         [span("special")]
                    |   string_                         [span("string")]
                    |   char_                           [span("char")]
                    |   number                          [span("number")]
                    |   ~cl::eps_p(cl::eol_p)
                    >>  u8_codepoint_p                  [unexpected_char]
                    ;

                macro =
                    // must not be followed by alpha or underscore
                    cl::eps_p(self.actions.state.macro
                        >> (cl::eps_p - (cl::alpha_p | '_')))
                    >> self.actions.state.macro
                                                        [do_macro]
                    ;

                escape =
                    cl::str_p("``")                     [pre_escape_back]
                    >>
                    (
                        (
                            (
                                (+(cl::anychar_p - "``") >> cl::eps_p("``"))
                                & g.phrase_start
                            )
                            >>  cl::str_p("``")
                        )
                        |
                        (
                            cl::eps_p                   [error]
                            >> *cl::anychar_p
                        )
                    )                                   [post_escape_back]
                    ;

                preprocessor
                    =   '#' >> *cl::space_p >> ((cl::alpha_p | '_') >> *(cl::alnum_p | '_'))
                    ;

                inline_callout
                    =   cl::confix_p(
                            "/*<" >> *cl::space_p,
                            (*cl::anychar_p)            [mark_text],
                            ">*/"
                        )
                        ;

                line_callout
                    =   cl::confix_p(
                            "/*<<" >> *cl::space_p,
                            (*cl::anychar_p)            [mark_text],
                            ">>*/"
                        )
                    >>  *cl::space_p
                    ;

                comment
                    =   cl::str_p("//")                 [span_start("comment")]
                    >>  *(  escape
                        |   (+(cl::anychar_p - (cl::eol_p | "``")))
                                                        [plain_char]
                        )
                    >>  cl::eps_p                       [span_end]
                    |   cl::str_p("/*")                 [span_start("comment")]
                    >>  *(  escape
                        |   (+(cl::anychar_p - (cl::str_p("*/") | "``")))
                                                        [plain_char]
                        )
                    >>  (!cl::str_p("*/"))              [span_end]
                    ;

                keyword
                    =   keywords.cpp >> (cl::eps_p - (cl::alnum_p | '_'))
                    ;   // make sure we recognize whole words only

                special
                    =   +cl::chset_p("~!%^&*()+={[}]:;,<.>?/|\\#-")
                    ;

                string_char = ('\\' >> u8_codepoint_p) | (cl::anychar_p - '\\');

                string_
                    =   !cl::as_lower_d['l'] >> cl::confix_p('"', *string_char, '"')
                    ;

                char_
                    =   !cl::as_lower_d['l'] >> cl::confix_p('\'', *string_char, '\'')
                    ;

                number
                    =   (
                            cl::as_lower_d["0x"] >> cl::hex_p
                        |   '0' >> cl::oct_p
                        |   cl::real_p
                        )
                        >>  *cl::as_lower_d[cl::chset_p("ldfu")]
                    ;

                identifier
                    =   (cl::alpha_p | '_') >> *(cl::alnum_p | '_')
                    ;
            }

            cl::rule<Scanner>
                            program, line_start, rest_of_line, macro, preprocessor,
                            inline_callout, line_callout, comment,
                            special, string_, 
                            char_, number, identifier, keyword, escape,
                            string_char;

            quickbook_grammar& g;

            cl::rule<Scanner> const&
            start() const { return program; }
        };

        syntax_highlight_actions& actions;
    };

    // Grammar for Python highlighting
    // See also: The Python Reference Manual
    // http://docs.python.org/ref/ref.html
    struct python_highlight : public cl::grammar<python_highlight>
    {
        python_highlight(syntax_highlight_actions& actions)
            : actions(actions) {}

        template <typename Scanner>
        struct definition
        {
            definition(python_highlight const& self)
                : g(self.actions.state.grammar())
            {
                member_action1<syntax_highlight_actions, char const*>
                    span(self.actions, &syntax_highlight_actions::span),
                    span_start(self.actions, &syntax_highlight_actions::span_start);
                member_action<syntax_highlight_actions>
                    span_end(self.actions, &syntax_highlight_actions::span_end),
                    unexpected_char(self.actions, &syntax_highlight_actions::unexpected_char),
                    plain_char(self.actions, &syntax_highlight_actions::plain_char),
                    pre_escape_back(self.actions, &syntax_highlight_actions::pre_escape_back),
                    post_escape_back(self.actions, &syntax_highlight_actions::post_escape_back),
                    mark_text(self.actions, &syntax_highlight_actions::mark_text),
                    callout(self.actions, &syntax_highlight_actions::callout);
                member_action_value<syntax_highlight_actions, std::string const&>
                    do_macro(self.actions, &syntax_highlight_actions::do_macro);
                error_action error(self.actions.state);

                program
                    =
                    *(  (+cl::space_p)                  [plain_char]
                    |   macro
                    |   escape          
                    |   comment
                    |   keyword                         [span("keyword")]
                    |   identifier                      [span("identifier")]
                    |   special                         [span("special")]
                    |   string_                         [span("string")]
                    |   number                          [span("number")]
                    |   u8_codepoint_p                  [unexpected_char]
                    )
                    ;

                macro = 
                    // must not be followed by alpha or underscore
                    cl::eps_p(self.actions.state.macro
                        >> (cl::eps_p - (cl::alpha_p | '_')))
                    >> self.actions.state.macro
                                                        [do_macro]
                    ;

                escape =
                    cl::str_p("``")                     [pre_escape_back]
                    >>
                    (
                        (
                            (
                                (+(cl::anychar_p - "``") >> cl::eps_p("``"))
                                & g.phrase_start
                            )
                            >>  cl::str_p("``")
                        )
                        |
                        (
                            cl::eps_p                   [error]
                            >> *cl::anychar_p
                        )
                    )                                   [post_escape_back]
                    ;

                comment
                    =   cl::str_p("#")                  [span_start("comment")]
                    >>  *(  escape
                        |   (+(cl::anychar_p - (cl::eol_p | "``")))
                                                        [plain_char]
                        )
                    >>  cl::eps_p                       [span_end]
                    ;

                keyword
                    =   keywords.python >> (cl::eps_p - (cl::alnum_p | '_'))
                    ;   // make sure we recognize whole words only

                special
                    =   +cl::chset_p("~!%^&*()+={[}]:;,<.>/|\\-")
                    ;

                string_prefix
                    =    cl::as_lower_d[cl::str_p("u") >> ! cl::str_p("r")]
                    ;
                
                string_
                    =   ! string_prefix >> (long_string | short_string)
                    ;

                string_char = ('\\' >> u8_codepoint_p) | (cl::anychar_p - '\\');
            
                short_string
                    =   cl::confix_p('\'', * string_char, '\'') |
                        cl::confix_p('"', * string_char, '"')
                    ;
            
                long_string
                    // Note: the "cl::str_p" on the next two lines work around
                    // an INTERNAL COMPILER ERROR when using VC7.1
                    =   cl::confix_p(cl::str_p("'''"), * string_char, "'''") |
                        cl::confix_p(cl::str_p("\"\"\""), * string_char, "\"\"\"")
                    ;
                
                number
                    =   (
                            cl::as_lower_d["0x"] >> cl::hex_p
                        |   '0' >> cl::oct_p
                        |   cl::real_p
                        )
                        >>  *cl::as_lower_d[cl::chset_p("lj")]
                    ;

                identifier
                    =   (cl::alpha_p | '_') >> *(cl::alnum_p | '_')
                    ;
            }

            cl::rule<Scanner>
                            program, macro, comment, special, string_, string_prefix, 
                            short_string, long_string, number, identifier, keyword, 
                            escape, string_char;

            quickbook_grammar& g;

            cl::rule<Scanner> const&
            start() const { return program; }
        };

        syntax_highlight_actions& actions;
    };

    // Grammar for plain text (no actual highlighting)
    struct teletype_highlight : public cl::grammar<teletype_highlight>
    {
        teletype_highlight(syntax_highlight_actions& actions)
            : actions(actions) {}

        template <typename Scanner>
        struct definition
        {
            definition(teletype_highlight const& self)
                : g(self.actions.state.grammar())
            {
                member_action<syntax_highlight_actions>
                    plain_char(self.actions, &syntax_highlight_actions::plain_char),
                    pre_escape_back(self.actions, &syntax_highlight_actions::pre_escape_back),
                    post_escape_back(self.actions, &syntax_highlight_actions::post_escape_back);
                member_action_value<syntax_highlight_actions, std::string const&>
                    do_macro(self.actions, &syntax_highlight_actions::do_macro);
                error_action error(self.actions.state);

                program
                    =
                    *(  macro
                    |   escape          
                    |   u8_codepoint_p                  [plain_char]
                    )
                    ;

                macro =
                    // must not be followed by alpha or underscore
                    cl::eps_p(self.actions.state.macro
                        >> (cl::eps_p - (cl::alpha_p | '_')))
                    >> self.actions.state.macro
                                                        [do_macro]
                    ;

                escape =
                    cl::str_p("``")                     [pre_escape_back]
                    >>
                    (
                        (
                            (
                                (+(cl::anychar_p - "``") >> cl::eps_p("``"))
                                & g.phrase_start
                            )
                            >>  cl::str_p("``")
                        )
                        |
                        (
                            cl::eps_p                   [error]
                            >> *cl::anychar_p
                        )
                    )                                   [post_escape_back]
                    ;
            }

            cl::rule<Scanner> program, macro, escape;

            quickbook_grammar& g;

            cl::rule<Scanner> const&
            start() const { return program; }
        };

        syntax_highlight_actions& actions;
    };

    void syntax_highlight(
        parse_iterator first,
        parse_iterator last,
        quickbook::state& state,
        std::string const& source_mode,
        bool is_block)
    {
        syntax_highlight_actions syn_actions(state, is_block);

        // print the code with syntax coloring
        if (source_mode == "c++")
        {
            cpp_highlight cpp_p(syn_actions);
            boost::spirit::classic::parse(first, last, cpp_p);
        }
        else if (source_mode == "python")
        {
            python_highlight python_p(syn_actions);
            boost::spirit::classic::parse(first, last, python_p);
        }
        else if (source_mode == "teletype")
        {
            teletype_highlight teletype_p(syn_actions);
            boost::spirit::classic::parse(first, last, teletype_p);
        }
        else
        {
            BOOST_ASSERT(0);
        }
    }
}
