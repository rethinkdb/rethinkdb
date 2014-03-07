/*=============================================================================
    Copyright (c) 2005 2006 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include "post_process.hpp"
#include <boost/spirit/include/classic_core.hpp>
#include <boost/bind.hpp>
#include <set>
#include <stack>
#include <cctype>

namespace quickbook
{
    namespace cl = boost::spirit::classic;
    typedef std::string::const_iterator iter_type;

    struct printer
    {
        printer(std::string& out, int& current_indent, int linewidth)
            : prev(0), out(out), current_indent(current_indent) , column(0)
            , in_string(false), linewidth(linewidth) {}

        void indent()
        {
            BOOST_ASSERT(current_indent >= 0); // this should not happen!
            for (int i = 0; i < current_indent; ++i)
                out += ' ';
            column = current_indent;
        }

        void trim_spaces()
        {
            out.erase(out.find_last_not_of(' ')+1); // trim trailing spaces
        }

        void break_line()
        {
            trim_spaces();
            out += '\n';
            indent();
        }

        bool line_is_empty() const
        {
            for (iter_type i = out.end()-(column-current_indent); i != out.end(); ++i)
            {
                if (*i != ' ')
                    return false;
            }
            return true;
        }

        void align_indent()
        {
            // make sure we are at the proper indent position
            if (column != current_indent)
            {
                if (column > current_indent)
                {
                    if (line_is_empty())
                    {
                        // trim just enough trailing spaces down to current_indent position
                        out.erase(out.end()-(column-current_indent), out.end());
                        column = current_indent;
                    }
                    else
                    {
                        // nope, line is not empty. do a hard CR
                        break_line();
                    }
                }
                else
                {
                    // will this happen? (i.e. column <= current_indent)
                    while (column != current_indent)
                    {
                        out += ' ';
                        ++column;
                    }
                }
            }
        }

        void print(char ch)
        {
            // Print a char. Attempt to break the line if we are exceeding
            // the target linewidth. The linewidth is not an absolute limit.
            // There are many cases where a line will exceed the linewidth
            // and there is no way to properly break the line. Preformatted
            // code that exceeds the linewidth are examples. We cannot break
            // preformatted code. We shall not attempt to be very strict with
            // line breaking. What's more important is to have a reproducable
            // output (i.e. processing two logically equivalent xml files
            // results in two lexically equivalent xml files). *** pretty
            // formatting is a secondary goal ***

            // Strings will occur only in tag attributes. Normal content
            // will have &quot; instead. We shall deal only with tag
            // attributes here.
            if (ch == '"')
                in_string = !in_string; // don't break strings!

            if (!in_string && std::isspace(static_cast<unsigned char>(ch)))
            {
                // we can break spaces if they are not inside strings
                if (!std::isspace(static_cast<unsigned char>(prev)))
                {
                    if (column >= linewidth)
                    {
                        break_line();
                        if (column == 0 && ch == ' ')
                        {
                            ++column;
                            out += ' ';
                        }
                    }
                    else
                    {
                        ++column;
                        out += ' ';
                    }
                }
            }
            else
            {
                // we can break tag boundaries and stuff after
                // delimiters if they are not inside strings
                // and *only-if* the preceding char is a space
                if (!in_string
                    && column >= linewidth
                    && (ch == '<' && std::isspace(static_cast<unsigned char>(prev))))
                    break_line();
                out += ch;
                ++column;
            }

            prev = ch;
        }

        void
        print(iter_type f, iter_type l)
        {
            for (iter_type i = f; i != l; ++i)
                print(*i);
        }

        void
        print_tag(iter_type f, iter_type l, bool is_flow_tag)
        {
            if (is_flow_tag)
            {
                print(f, l);
            }
            else
            {
                // This is not a flow tag, so, we're going to do a
                // carriage return anyway. Let us remove extra right
                // spaces.
                std::string str(f, l);
                BOOST_ASSERT(f != l); // this should not happen
                iter_type i = str.end();
                while (i != str.begin() && std::isspace(static_cast<unsigned char>(*(i-1))))
                    --i;
                print(str.begin(), i);
            }
        }

        char prev;
        std::string& out;
        int& current_indent;
        int column;
        bool in_string;
        int linewidth;
    };

    char const* block_tags_[] =
    {
          "author"
        , "blockquote"
        , "bridgehead"
        , "callout"
        , "calloutlist"
        , "caution"
        , "copyright"
        , "entry"
        , "important"
        , "informaltable"
        , "itemizedlist"
        , "legalnotice"
        , "listitem"
        , "note"
        , "orderedlist"
        , "para"
        , "row"
        , "section"
        , "simpara"
        , "table"
        , "tbody"
        , "textobject"
        , "tgroup"
        , "thead"
        , "tip"
        , "variablelist"
        , "varlistentry"
        , "warning"
        , "xml"
        , "xi:include"
    };

    char const* doc_types_[] =
    {
          "book"
        , "article"
        , "library"
        , "chapter"
        , "part"
        , "appendix"
        , "preface"
        , "qandadiv"
        , "qandaset"
        , "reference"
        , "set"
    };

    struct tidy_compiler
    {
        tidy_compiler(std::string& out, int linewidth)
            : out(out), current_indent(0), printer_(out, current_indent, linewidth)
        {
            static int const n_block_tags = sizeof(block_tags_)/sizeof(char const*);
            for (int i = 0; i != n_block_tags; ++i)
            {
                block_tags.insert(block_tags_[i]);
            }

            static int const n_doc_types = sizeof(doc_types_)/sizeof(char const*);
            for (int i = 0; i != n_doc_types; ++i)
            {
                block_tags.insert(doc_types_[i]);
                block_tags.insert(doc_types_[i] + std::string("info"));
                block_tags.insert(doc_types_[i] + std::string("purpose"));
            }
        }

        bool is_flow_tag(std::string const& tag)
        {
            return block_tags.find(tag) == block_tags.end();
        }

        std::set<std::string> block_tags;
        std::stack<std::string> tags;
        std::string& out;
        int current_indent;
        printer printer_;
        std::string current_tag;
    };

    struct tidy_grammar : cl::grammar<tidy_grammar>
    {
        tidy_grammar(tidy_compiler& state, int indent)
            : state(state), indent(indent) {}

        template <typename Scanner>
        struct definition
        {
            definition(tidy_grammar const& self)
            {
                tag = (cl::lexeme_d[+(cl::alpha_p | '_' | ':')])  [boost::bind(&tidy_grammar::do_tag, &self, _1, _2)];

                code =
                        "<programlisting>"
                    >>  *(cl::anychar_p - "</programlisting>")
                    >>  "</programlisting>"
                    ;

                // What's the business of cl::lexeme_d['>' >> *cl::space_p]; ?
                // It is there to preserve the space after the tag that is
                // otherwise consumed by the cl::space_p skipper.

                escape =
                    cl::str_p("<!--quickbook-escape-prefix-->") >>
                    (*(cl::anychar_p - cl::str_p("<!--quickbook-escape-postfix-->")))
                    [
                        boost::bind(&tidy_grammar::do_escape, &self, _1, _2)
                    ]
                    >>  cl::lexeme_d
                        [
                            cl::str_p("<!--quickbook-escape-postfix-->") >>
                            (*cl::space_p)
                            [
                                boost::bind(&tidy_grammar::do_escape_post, &self, _1, _2)
                            ]
                        ]
                    ;

                start_tag = '<' >> tag >> *(cl::anychar_p - '>') >> cl::lexeme_d['>' >> *cl::space_p];
                start_end_tag =
                        '<' >> tag >> *(cl::anychar_p - ("/>" | cl::ch_p('>'))) >> cl::lexeme_d["/>" >> *cl::space_p]
                    |   "<?" >> tag >> *(cl::anychar_p - '?') >> cl::lexeme_d["?>" >> *cl::space_p]
                    |   "<!--" >> *(cl::anychar_p - "-->") >> cl::lexeme_d["-->" >> *cl::space_p]
                    |   "<!" >> tag >> *(cl::anychar_p - '>') >> cl::lexeme_d['>' >> *cl::space_p]
                    ;
                content = cl::lexeme_d[ +(cl::anychar_p - '<') ];
                end_tag = "</" >> +(cl::anychar_p - '>') >> cl::lexeme_d['>' >> *cl::space_p];

                markup =
                        escape
                    |   code            [boost::bind(&tidy_grammar::do_code, &self, _1, _2)]
                    |   start_end_tag   [boost::bind(&tidy_grammar::do_start_end_tag, &self, _1, _2)]
                    |   start_tag       [boost::bind(&tidy_grammar::do_start_tag, &self, _1, _2)]
                    |   end_tag         [boost::bind(&tidy_grammar::do_end_tag, &self, _1, _2)]
                    |   content         [boost::bind(&tidy_grammar::do_content, &self, _1, _2)]
                    ;

                tidy = +markup;
            }

            cl::rule<Scanner> const&
            start() { return tidy; }

            cl::rule<Scanner>
                            tidy, tag, start_tag, start_end_tag,
                            content, end_tag, markup, code, escape;
        };

        void do_escape_post(iter_type f, iter_type l) const
        {
            for (iter_type i = f; i != l; ++i)
                state.out += *i;
        }

        void do_escape(iter_type f, iter_type l) const
        {
            while (f != l && std::isspace(*f))
                ++f;
            for (iter_type i = f; i != l; ++i)
                state.out += *i;
        }

        void do_code(iter_type f, iter_type l) const
        {
            state.printer_.trim_spaces();
            if (state.out[state.out.size() - 1] != '\n')
                state.out += '\n';
            // print the string taking care of line
            // ending CR/LF platform issues
            for (iter_type i = f; i != l; ++i)
            {
                if (*i == '\n')
                {
                    state.printer_.trim_spaces();
                    state.out += '\n';
                    ++i;
                    if (i != l && *i != '\r')
                        state.out += *i;
                }
                else if (*i == '\r')
                {
                    state.printer_.trim_spaces();
                    state.out += '\n';
                    ++i;
                    if (i != l && *i != '\n')
                        state.out += *i;
                }
                else
                {
                    state.out += *i;
                }
            }
            state.out += '\n';
            state.printer_.indent();
        }

        void do_tag(iter_type f, iter_type l) const
        {
            state.current_tag = std::string(f, l);
        }

        void do_start_end_tag(iter_type f, iter_type l) const
        {
            bool is_flow_tag = state.is_flow_tag(state.current_tag);
            if (!is_flow_tag)
                state.printer_.align_indent();
            state.printer_.print_tag(f, l, is_flow_tag);
            if (!is_flow_tag)
                state.printer_.break_line();
        }

        void do_start_tag(iter_type f, iter_type l) const
        {
            state.tags.push(state.current_tag);
            bool is_flow_tag = state.is_flow_tag(state.current_tag);
            if (!is_flow_tag)
                state.printer_.align_indent();
            state.printer_.print_tag(f, l, is_flow_tag);
            if (!is_flow_tag)
            {
                state.current_indent += indent;
                state.printer_.break_line();
            }
        }

        void do_content(iter_type f, iter_type l) const
        {
            state.printer_.print(f, l);
        }

        void do_end_tag(iter_type f, iter_type l) const
        {
            if (state.tags.empty())
                throw quickbook::post_process_failure("Mismatched tags.");
        
            bool is_flow_tag = state.is_flow_tag(state.tags.top());
            if (!is_flow_tag)
            {
                state.current_indent -= indent;
                state.printer_.align_indent();
            }
            state.printer_.print_tag(f, l, is_flow_tag);
            if (!is_flow_tag)
                state.printer_.break_line();
            state.tags.pop();
        }

        tidy_compiler& state;
        int indent;
    };

    std::string post_process(
        std::string const& in
      , int indent
      , int linewidth)
    {
        if (indent == -1)
            indent = 2;         // set default to 2
        if (linewidth == -1)
            linewidth = 80;     // set default to 80

        std::string tidy;
        tidy_compiler state(tidy, linewidth);
        tidy_grammar g(state, indent);
        cl::parse_info<iter_type> r = parse(in.begin(), in.end(), g, cl::space_p);
        if (r.full)
        {
            return tidy;
        }
        else
        {
            throw quickbook::post_process_failure("Post Processing Failed.");
        }
    }
}

