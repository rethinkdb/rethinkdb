// ----------------------------------------------------------------------------
// Copyright (C) 2002-2007 Marcin Kalicinski
// Copyright (C) 2007 Alexey Baskakov
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_DETAIL_XML_PARSER_WRITER_SETTINGS_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_DETAIL_XML_PARSER_WRITER_SETTINGS_HPP_INCLUDED

#include <string>
#include <boost/property_tree/detail/ptree_utils.hpp>

namespace boost { namespace property_tree { namespace xml_parser
{
    
    // Naively convert narrow string to another character type
    template<class Ch>
    std::basic_string<Ch> widen(const char *text)
    {
        std::basic_string<Ch> result;
        while (*text)
        {
            result += Ch(*text);
            ++text;
        }
        return result;
    }

    //! Xml writer settings. The default settings lead to no pretty printing.
    template<class Ch>
    class xml_writer_settings
    {
    public:
        xml_writer_settings(Ch inchar = Ch(' '),
                typename std::basic_string<Ch>::size_type incount = 0,
                const std::basic_string<Ch> &enc = widen<Ch>("utf-8"))
            : indent_char(inchar)
            , indent_count(incount)
            , encoding(enc)
        {
        }

        Ch indent_char;
        typename std::basic_string<Ch>::size_type indent_count;
        std::basic_string<Ch> encoding;
    };

    template <class Ch>
    xml_writer_settings<Ch> xml_writer_make_settings(Ch indent_char = Ch(' '),
        typename std::basic_string<Ch>::size_type indent_count = 0,
        const std::basic_string<Ch> &encoding = widen<Ch>("utf-8"))
    {
        return xml_writer_settings<Ch>(indent_char, indent_count, encoding);
    }

} } }

#endif
