/*=============================================================================
    Copyright (c) 2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_SPIRIT_QUICKBOOK_TEMPLATE_TAGS_HPP)
#define BOOST_SPIRIT_QUICKBOOK_TEMPLATE_TAGS_HPP

#include "value_tags.hpp"

namespace quickbook
{
    QUICKBOOK_VALUE_TAGS(template_tags, 0x100,
        (template_)
        (escape)
        (identifier)
        (block)
        (phrase)
        (snippet)
    )
}

#endif
