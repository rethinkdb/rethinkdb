/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "lexer.hpp"

namespace client { namespace lexer
{
    template <typename BaseIterator>
    conjure_tokens<BaseIterator>::conjure_tokens()
      : identifier("[a-zA-Z_][a-zA-Z_0-9]*", token_ids::identifier)
      , lit_uint("[0-9]+", token_ids::lit_uint)
      , true_or_false("true|false", token_ids::true_or_false)
    {
        lex::_pass_type _pass;

        this->self = lit_uint | true_or_false;

        add_keyword("void");
        add_keyword("int");
        add_keyword("if");
        add_keyword("else");
        add_keyword("while");
        add_keyword("return");

        this->self.add
                ("\\|\\|", token_ids::logical_or)
                ("&&", token_ids::logical_and)
                ("==", token_ids::equal)
                ("!=", token_ids::not_equal)
                ("<", token_ids::less)
                ("<=", token_ids::less_equal)
                (">", token_ids::greater)
                (">=", token_ids::greater_equal)
                ("\\+", token_ids::plus)
                ("\\-", token_ids::minus)
                ("\\*", token_ids::times)
                ("\\/", token_ids::divide)
                ("!", token_ids::not_)
            ;

        this->self += lex::char_('(') | ')' | '{' | '}' | ',' | '=' | ';';

        this->self +=
                identifier
            |   lex::string("\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/", token_ids::comment)
                [
                    lex::_pass = lex::pass_flags::pass_ignore
                ]
            |   lex::string("[ \t\n\r]+", token_ids::whitespace)
                [
                    lex::_pass = lex::pass_flags::pass_ignore
                ]
            ;
    }

    template <typename BaseIterator>
    bool conjure_tokens<BaseIterator>::add_keyword(std::string const& keyword)
    {
        // add the token to the lexer
        token_ids::type id = token_ids::type(this->get_next_id());
        this->self.add(keyword, id);

        // store the mapping for later retrieval
        std::pair<typename keyword_map_type::iterator, bool> p =
            keywords_.insert(typename keyword_map_type::value_type(keyword, id));

        return p.second;
    }
}}

