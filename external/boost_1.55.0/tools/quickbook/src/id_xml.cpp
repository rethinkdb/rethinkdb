/*=============================================================================
    Copyright (c) 2011-2013 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "id_manager_impl.hpp"
#include "utils.hpp"
#include <boost/range/algorithm.hpp>

namespace quickbook
{
    namespace
    {
        char const* id_attributes_[] =
        {
            "id",
            "linkend",
            "linkends",
            "arearefs"
        };
    }

    xml_processor::xml_processor()
    {
        static int const n_id_attributes = sizeof(id_attributes_)/sizeof(char const*);
        for (int i = 0; i != n_id_attributes; ++i)
        {
            id_attributes.push_back(id_attributes_[i]);
        }

        boost::sort(id_attributes);
    }

    template <typename Iterator>
    bool read(Iterator& it, Iterator end, char const* text)
    {
        for(Iterator it2 = it;; ++it2, ++text) {
            if (!*text) {
                it = it2;
                return true;
            }

            if (it2 == end || *it2 != *text)
                return false;
        }
    }

    template <typename Iterator>
    void read_past(Iterator& it, Iterator end, char const* text)
    {
        while (it != end && !read(it, end, text)) ++it;
    }

    bool find_char(char const* text, char c)
    {
        for(;*text; ++text)
            if (c == *text) return true;
        return false;
    }

    template <typename Iterator>
    void read_some_of(Iterator& it, Iterator end, char const* text)
    {
        while(it != end && find_char(text, *it)) ++it;
    }

    template <typename Iterator>
    void read_to_one_of(Iterator& it, Iterator end, char const* text)
    {
        while(it != end && !find_char(text, *it)) ++it;
    }

    void xml_processor::parse(boost::string_ref source, callback& c)
    {
        typedef boost::string_ref::const_iterator iterator;

        c.start(source);

        iterator it = source.begin(), end = source.end();

        for(;;)
        {
            read_past(it, end, "<");
            if (it == end) break;

            if (read(it, end, "!--quickbook-escape-prefix-->"))
            {
                read_past(it, end, "<!--quickbook-escape-postfix-->");
                continue;
            }

            switch(*it)
            {
            case '?':
                ++it;
                read_past(it, end, "?>");
                break;

            case '!':
                if (read(it, end, "!--"))
                    read_past(it, end, "-->");
                else
                    read_past(it, end, ">");
                break;

            default:
                if ((*it >= 'a' && *it <= 'z') ||
                        (*it >= 'A' && *it <= 'Z') ||
                        *it == '_' || *it == ':')
                {
                    read_to_one_of(it, end, " \t\n\r>");

                    for (;;) {
                        read_some_of(it, end, " \t\n\r");
                        iterator name_start = it;
                        read_to_one_of(it, end, "= \t\n\r>");
                        if (it == end || *it == '>') break;
                        boost::string_ref name(name_start, it - name_start);
                        ++it;

                        read_some_of(it, end, "= \t\n\r");
                        if (it == end || (*it != '"' && *it != '\'')) break;

                        char delim = *it;
                        ++it;

                        iterator value_start = it;

                        it = std::find(it, end, delim);
                        if (it == end) break;
                        boost::string_ref value(value_start, it - value_start);
                        ++it;

                        if (boost::find(id_attributes, detail::to_s(name))
                                != id_attributes.end())
                        {
                            c.id_value(value);
                        }
                    }
                }
                else
                {
                    read_past(it, end, ">");
                }
            }
        }

        c.finish(source);
    }
}
