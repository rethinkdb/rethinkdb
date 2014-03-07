/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_MARKUPS_HPP)
#define BOOST_SPIRIT_MARKUPS_HPP

#include <iosfwd>
#include "values.hpp"

namespace quickbook
{
    namespace detail
    {    
        struct markup {
            value::tag_type tag;
            char const* pre;
            char const* post;
        };
        
        markup const& get_markup(value::tag_type);
        std::ostream& operator<<(std::ostream&, markup const&);
    }
}

#endif // BOOST_SPIRIT_MARKUPS_HPP

