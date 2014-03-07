/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    Copyright (c) 2010 Daniel James
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_FWD_HPP)
#define BOOST_SPIRIT_FWD_HPP

#include "iterator.hpp"
#include <boost/intrusive_ptr.hpp>
#include <boost/utility/string_ref.hpp>

namespace quickbook
{
    struct state;
    struct quickbook_grammar;
    struct collector;
    struct id_manager;
    struct section_info;
    struct file;
    struct template_symbol;
    typedef boost::intrusive_ptr<file> file_ptr;

    typedef boost::string_ref::const_iterator string_iterator;
    typedef lookback_iterator<string_iterator> parse_iterator;

    inline void ignore_variable(void const*) {} 
}

#endif
