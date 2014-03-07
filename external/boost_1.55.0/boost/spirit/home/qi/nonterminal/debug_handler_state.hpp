/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_DEBUG_HANDLER_STATE_APR_21_2010_0733PM)
#define BOOST_SPIRIT_DEBUG_HANDLER_STATE_APR_21_2010_0733PM

#if defined(_MSC_VER)
#pragma once
#endif

namespace boost { namespace spirit { namespace qi
{
    enum debug_handler_state
    {
        pre_parse
      , successful_parse
      , failed_parse
    };
}}}

#endif
