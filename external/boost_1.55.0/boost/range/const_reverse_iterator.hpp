// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_CONST_REVERSE_ITERATOR_HPP
#define BOOST_RANGE_CONST_REVERSE_ITERATOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include <boost/range/reverse_iterator.hpp>

namespace boost
{
    //
    // This interface is deprecated, use range_reverse_iterator<const T>
    //
    
    template< typename C >
    struct range_const_reverse_iterator : range_reverse_iterator<const C>
    { };
    
} // namespace boost

#endif
