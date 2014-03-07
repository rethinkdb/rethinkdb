/*=============================================================================
    Copyright (c) 2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_SPIRIT_QUICKBOOK_TABLE_TAGS_HPP)
#define BOOST_SPIRIT_QUICKBOOK_TABLE_TAGS_HPP

#include "value_tags.hpp"

namespace quickbook
{
    QUICKBOOK_VALUE_TAGS(block_tags, 0x200,
        (begin_section)(end_section)
        (generic_heading)
        (heading1)(heading2)(heading3)(heading4)(heading5)(heading6)
        (blurb)(blockquote)(preformatted)
        (warning)(caution)(important)(note)(tip)(block)
        (macro_definition)(template_definition)
        (variable_list)(table)
        (xinclude)(import)(include)
        (paragraph)(paragraph_in_list)
        (ordered_list)(itemized_list)
        (hr)
    )

    QUICKBOOK_VALUE_TAGS(table_tags, 0x250,
        (title)(row)
    )

    QUICKBOOK_VALUE_TAGS(general_tags, 0x300,
        (element_id)(include_id)(list_indent)(list_mark)
    )

}

#endif
