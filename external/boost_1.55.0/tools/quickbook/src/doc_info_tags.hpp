/*=============================================================================
    Copyright (c) 2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_SPIRIT_QUICKBOOK_DOC_INFO_TAGS_HPP)
#define BOOST_SPIRIT_QUICKBOOK_DOC_INFO_TAGS_HPP

#include "value_tags.hpp"

namespace quickbook
{
    QUICKBOOK_VALUE_TAGS(doc_info_tags, 0x400,
        (before_docinfo)
        (type)
        (title)
        (author_surname)(author_first)
        (copyright_year)(copyright_year_end)(copyright_name)
        (license)
        (biblioid_class)(biblioid_value)
        (escaped_attribute)
    )

    QUICKBOOK_VALUE_NAMED_TAGS(doc_attributes, 0x440,
        ((qbk_version)("quickbook"))
        ((compatibility_mode)("compatibility-mode"))
        ((source_mode)("source-mode"))
    )

    QUICKBOOK_VALUE_NAMED_TAGS(doc_info_attributes, 0x450,
        ((id)("id"))
        ((dirname)("dirname"))
        ((last_revision)("last-revision"))
        ((purpose)("purpose"))
        ((category)("category"))
        ((lang)("lang"))
        ((version)("version"))
        ((authors)("authors"))
        ((copyright)("copyright"))
        ((license)("license"))
        ((biblioid)("biblioid"))
        ((xmlbase)("xmlbase"))
    )
}

#endif
