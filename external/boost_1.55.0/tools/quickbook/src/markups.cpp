/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "quickbook.hpp"
#include "markups.hpp"
#include "block_tags.hpp"
#include "phrase_tags.hpp"
#include <boost/foreach.hpp>
#include <ostream>
#include <map>

namespace quickbook
{
    namespace detail
    {
        std::map<value::tag_type, markup> markups;

        void initialise_markups()
        {
            markup init_markups[] = {
                { block_tags::paragraph, "<para>\n", "</para>\n" },
                { block_tags::paragraph_in_list, "<simpara>\n", "</simpara>\n" },
                { block_tags::blurb, "<sidebar role=\"blurb\">\n", "</sidebar>\n" },
                { block_tags::blockquote, "<blockquote>", "</blockquote>" },
                { block_tags::preformatted, "<programlisting>", "</programlisting>" },
                { block_tags::warning, "<warning>", "</warning>" },
                { block_tags::caution, "<caution>", "</caution>" },
                { block_tags::important, "<important>", "</important>" },
                { block_tags::note, "<note>", "</note>" },
                { block_tags::tip, "<tip>", "</tip>" },
                { block_tags::block, "", "" },
                { block_tags::ordered_list, "<orderedlist>", "</orderedlist>" },
                { block_tags::itemized_list, "<itemizedlist>", "</itemizedlist>" },
                { block_tags::hr, "<para/>", 0 },
                { phrase_tags::url, "<ulink url=\"", "</ulink>" },
                { phrase_tags::link, "<link linkend=\"", "</link>" },
                { phrase_tags::funcref, "<functionname alt=\"", "</functionname>" },
                { phrase_tags::classref, "<classname alt=\"", "</classname>" },
                { phrase_tags::memberref, "<methodname alt=\"", "</methodname>" },
                { phrase_tags::enumref, "<enumname alt=\"", "</enumname>" },
                { phrase_tags::macroref, "<macroname alt=\"", "</macroname>" },
                { phrase_tags::headerref, "<headername alt=\"", "</headername>" },
                { phrase_tags::conceptref, "<conceptname alt=\"", "</conceptname>" },
                { phrase_tags::globalref, "<globalname alt=\"", "</globalname>" },
                { phrase_tags::bold, "<emphasis role=\"bold\">", "</emphasis>" },
                { phrase_tags::italic, "<emphasis>", "</emphasis>" },
                { phrase_tags::underline, "<emphasis role=\"underline\">", "</emphasis>" },
                { phrase_tags::teletype, "<literal>", "</literal>" },
                { phrase_tags::strikethrough, "<emphasis role=\"strikethrough\">", "</emphasis>" },
                { phrase_tags::quote, "<quote>", "</quote>" },
                { phrase_tags::replaceable, "<replaceable>", "</replaceable>" },
                { phrase_tags::escape, "<!--quickbook-escape-prefix-->", "<!--quickbook-escape-postfix-->" },
                { phrase_tags::break_mark, "<sbr/>\n", 0 }
            };

            BOOST_FOREACH(markup m, init_markups)
            {
                markups[m.tag] = m;
            }
        }

        markup const& get_markup(value::tag_type t)
        {
            return markups[t];
        }

        std::ostream& operator<<(std::ostream& out, markup const& m)
        {
            return out<<"{"<<m.tag<<": \""<<m.pre<<"\", \""<<m.post<<"\"}";
        }
    }
}
