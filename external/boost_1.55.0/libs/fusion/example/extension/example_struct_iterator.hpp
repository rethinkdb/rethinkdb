/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_EXAMPLE_STRUCT_ITERATOR)
#define BOOST_FUSION_EXAMPLE_STRUCT_ITERATOR

#include <boost/fusion/support/iterator_base.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/fusion/support/tag_of_fwd.hpp>
#include <boost/mpl/int.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/static_assert.hpp>

#include "./detail/next_impl.hpp"
#include "./detail/prior_impl.hpp"
#include "./detail/deref_impl.hpp"
#include "./detail/advance_impl.hpp"
#include "./detail/distance_impl.hpp"
#include "./detail/value_of_impl.hpp"
#include "./detail/equal_to_impl.hpp"
#include "./detail/key_of_impl.hpp"
#include "./detail/value_of_data_impl.hpp"
#include "./detail/deref_data_impl.hpp"

namespace example
{
    struct example_struct_iterator_tag;

    template<typename Struct, int Pos>
    struct example_struct_iterator;
}

namespace boost { namespace fusion {

    namespace traits
    {
        template<typename Struct, int Pos>
        struct tag_of<example::example_struct_iterator<Struct, Pos> >
        {
            typedef example::example_struct_iterator_tag type;
        };
    }
}}
    
namespace example {
    template<typename Struct, int Pos>
    struct example_struct_iterator
        : boost::fusion::iterator_base<example_struct_iterator<Struct, Pos> >
    {
        BOOST_STATIC_ASSERT(Pos >=0 && Pos < 3);
        typedef Struct struct_type;
        typedef boost::mpl::int_<Pos> index;

        struct category
          : boost::fusion::random_access_traversal_tag
          , boost::fusion::associative_tag
        {};

        example_struct_iterator(Struct& str)
            : struct_(str) {}

        Struct& struct_;
    };
}

#endif
