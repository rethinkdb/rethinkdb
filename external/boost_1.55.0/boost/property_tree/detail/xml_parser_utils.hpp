// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_DETAIL_XML_PARSER_UTILS_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_DETAIL_XML_PARSER_UTILS_HPP_INCLUDED

#include <boost/property_tree/detail/ptree_utils.hpp>
#include <boost/property_tree/detail/xml_parser_error.hpp>
#include <boost/property_tree/detail/xml_parser_writer_settings.hpp>
#include <string>
#include <algorithm>
#include <locale>

namespace boost { namespace property_tree { namespace xml_parser
{

    template<class Ch>
    std::basic_string<Ch> condense(const std::basic_string<Ch> &s)
    {
        std::basic_string<Ch> r;
        std::locale loc;
        bool space = false;
        typename std::basic_string<Ch>::const_iterator end = s.end();
        for (typename std::basic_string<Ch>::const_iterator it = s.begin();
             it != end; ++it)
        {
            if (isspace(*it, loc) || *it == Ch('\n'))
            {
                if (!space)
                    r += Ch(' '), space = true;
            }
            else
                r += *it, space = false;
        }
        return r;
    }

    template<class Ch>
    std::basic_string<Ch> encode_char_entities(const std::basic_string<Ch> &s)
    {
        // Don't do anything for empty strings.
        if(s.empty()) return s;

        typedef typename std::basic_string<Ch> Str;
        Str r;
        // To properly round-trip spaces and not uglify the XML beyond
        // recognition, we have to encode them IF the text contains only spaces.
        Str sp(1, Ch(' '));
        if(s.find_first_not_of(sp) == Str::npos) {
            // The first will suffice.
            r = detail::widen<Ch>("&#32;");
            r += Str(s.size() - 1, Ch(' '));
        } else {
            typename Str::const_iterator end = s.end();
            for (typename Str::const_iterator it = s.begin(); it != end; ++it)
            {
                switch (*it)
                {
                    case Ch('<'): r += detail::widen<Ch>("&lt;"); break;
                    case Ch('>'): r += detail::widen<Ch>("&gt;"); break;
                    case Ch('&'): r += detail::widen<Ch>("&amp;"); break;
                    case Ch('"'): r += detail::widen<Ch>("&quot;"); break;
                    case Ch('\''): r += detail::widen<Ch>("&apos;"); break;
                    default: r += *it; break;
                }
            }
        }
        return r;
    }
    
    template<class Ch>
    std::basic_string<Ch> decode_char_entities(const std::basic_string<Ch> &s)
    {
        typedef typename std::basic_string<Ch> Str;
        Str r;
        typename Str::const_iterator end = s.end();
        for (typename Str::const_iterator it = s.begin(); it != end; ++it)
        {
            if (*it == Ch('&'))
            {
                typename Str::const_iterator semicolon = std::find(it + 1, end, Ch(';'));
                if (semicolon == end)
                    BOOST_PROPERTY_TREE_THROW(xml_parser_error("invalid character entity", "", 0));
                Str ent(it + 1, semicolon);
                if (ent == detail::widen<Ch>("lt")) r += Ch('<');
                else if (ent == detail::widen<Ch>("gt")) r += Ch('>');
                else if (ent == detail::widen<Ch>("amp")) r += Ch('&');
                else if (ent == detail::widen<Ch>("quot")) r += Ch('"');
                else if (ent == detail::widen<Ch>("apos")) r += Ch('\'');
                else
                    BOOST_PROPERTY_TREE_THROW(xml_parser_error("invalid character entity", "", 0));
                it = semicolon;
            }
            else
                r += *it;
        }
        return r;
    }
    
    template<class Ch>
    const std::basic_string<Ch> &xmldecl()
    {
        static std::basic_string<Ch> s = detail::widen<Ch>("<?xml>");
        return s;
    }

    template<class Ch>
    const std::basic_string<Ch> &xmlattr()
    {
        static std::basic_string<Ch> s = detail::widen<Ch>("<xmlattr>");
        return s;
    }

    template<class Ch>
    const std::basic_string<Ch> &xmlcomment()
    {
        static std::basic_string<Ch> s = detail::widen<Ch>("<xmlcomment>");
        return s;
    }

    template<class Ch>
    const std::basic_string<Ch> &xmltext()
    {
        static std::basic_string<Ch> s = detail::widen<Ch>("<xmltext>");
        return s;
    }

} } }

#endif
