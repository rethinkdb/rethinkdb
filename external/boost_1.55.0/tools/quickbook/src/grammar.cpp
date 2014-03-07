/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    Copyright (c) 2010 Daniel James
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "grammar_impl.hpp"

namespace quickbook
{
   quickbook_grammar::quickbook_grammar(quickbook::state& s)
        : impl_(new impl(s))
        , command_line_macro(impl_->command_line, "command_line_macro")
        , inline_phrase(impl_->inline_phrase, "inline_phrase")
        , phrase_start(impl_->phrase_start, "phrase")
        , block_start(impl_->block_start, "block")
        , doc_info(impl_->doc_info_details, "doc_info")
    {
    }
    
    quickbook_grammar::~quickbook_grammar()
    {
    }

    quickbook_grammar::impl::impl(quickbook::state& s)
        : state(s)
        , cleanup_()
    {
        init_main();
        init_block_elements();
        init_phrase_elements();
        init_doc_info();
    }
}
