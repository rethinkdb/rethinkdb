/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_SINGLE_VIEW_ITERATOR_05052005_0340)
#define BOOST_FUSION_SINGLE_VIEW_ITERATOR_05052005_0340

#include <boost/fusion/support/detail/access.hpp>
#include <boost/fusion/support/iterator_base.hpp>
#include <boost/fusion/view/single_view/detail/deref_impl.hpp>
#include <boost/fusion/view/single_view/detail/next_impl.hpp>
#include <boost/fusion/view/single_view/detail/prior_impl.hpp>
#include <boost/fusion/view/single_view/detail/advance_impl.hpp>
#include <boost/fusion/view/single_view/detail/distance_impl.hpp>
#include <boost/fusion/view/single_view/detail/equal_to_impl.hpp>
#include <boost/fusion/view/single_view/detail/value_of_impl.hpp>
#include <boost/config.hpp>

#if defined (BOOST_MSVC)
#  pragma warning(push)
#  pragma warning (disable: 4512) // assignment operator could not be generated.
#endif

namespace boost { namespace fusion
{
    struct single_view_iterator_tag;
    struct random_access_traversal_tag;

    template <typename SingleView, typename Pos>
    struct single_view_iterator
        : iterator_base<single_view_iterator<SingleView, Pos> >
    {
        typedef single_view_iterator_tag fusion_tag;
        typedef random_access_traversal_tag category;
        typedef typename SingleView::value_type value_type;
        typedef Pos position;
        typedef SingleView single_view_type;

        explicit single_view_iterator(single_view_type& in_view)
            : view(in_view) {}

        SingleView& view;

    private:
        single_view_iterator& operator=(single_view_iterator const&);
    };
}}

#if defined (BOOST_MSVC)
#  pragma warning(pop)
#endif

#endif


