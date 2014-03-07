// Boost.Range library
//
//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_HAS_ITERATOR_HPP_INCLUDED
#define BOOST_RANGE_HAS_ITERATOR_HPP_INCLUDED

#include <boost/mpl/bool.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/range/iterator.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost
{
    namespace range_detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(type)

        template<class T, class Enabler = void>
        struct has_range_iterator_impl
            : boost::mpl::false_
        {
        };

        template<class T>
        struct has_range_iterator_impl<T, BOOST_DEDUCED_TYPENAME enable_if< has_type< range_mutable_iterator<T> > >::type>
            : boost::mpl::true_
        {
        };

        template<class T, class Enabler = void>
        struct has_range_const_iterator_impl
            : boost::mpl::false_
        {
        };

        template<class T>
        struct has_range_const_iterator_impl<T, BOOST_DEDUCED_TYPENAME enable_if< has_type< range_const_iterator<T> > >::type>
            : boost::mpl::true_
        {
        };

    } // namespace range_detail

    template<class T>
    struct has_range_iterator
        : range_detail::has_range_iterator_impl<T>
    {};

    template<class T>
    struct has_range_const_iterator
        : range_detail::has_range_const_iterator_impl<T>
    {};
} // namespace boost

#endif // include guard

