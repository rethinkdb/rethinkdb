/*=============================================================================
    Copyright (c) 2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_SPIRIT_QUICKBOOK_PHRASE_TAGS_HPP)
#define BOOST_SPIRIT_QUICKBOOK_PHRASE_TAGS_HPP

#include "value_tags.hpp"

namespace quickbook
{
    QUICKBOOK_VALUE_TAGS(phrase_tags, 0x500,
        (image)
        (url)(link)(anchor)
        (funcref)(classref)(memberref)(enumref)
        (macroref)(headerref)(conceptref)(globalref)
        (bold)(italic)(underline)(teletype)(strikethrough)(quote)(replaceable)
        (footnote)
        (escape)
        (break_mark)
        (role)
    )
    
    QUICKBOOK_VALUE_NAMED_TAGS(source_mode_tags, 0x550,
        ((cpp)("c++"))
        ((python)("python"))
        ((teletype)("teletype"))
    )

    QUICKBOOK_VALUE_TAGS(code_tags, 0x560,
        (code_block)
        (inline_code)
        (inline_code_block)
        (next_source_mode)
    )
}

#endif
