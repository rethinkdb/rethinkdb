/*=============================================================================
    Copyright (c) 2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_QUICKBOOK_SCOPED_HPP)
#define BOOST_QUICKBOOK_SCOPED_HPP

#include <cassert>

namespace quickbook {

    struct scoped_action_base
    {
        bool start() { return true; }
        template <typename Iterator>
        void success(Iterator, Iterator) {}
        void failure() {}
        void cleanup() {}
        
        template <typename ResultT, typename ScannerT>
        bool result(ResultT result, ScannerT const&)
        {
            return result;
        }
    };
}

#endif
