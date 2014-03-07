/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if defined(_MSC_VER)
# pragma warning(disable: 4345)
#endif

#include "config.hpp"
#include "lexer.hpp"
#include "expression_def.hpp"

typedef std::string::const_iterator base_iterator_type;
typedef client::lexer::conjure_tokens<base_iterator_type> lexer_type;
typedef lexer_type::iterator_type iterator_type;

template struct client::parser::expression<iterator_type, lexer_type>;
