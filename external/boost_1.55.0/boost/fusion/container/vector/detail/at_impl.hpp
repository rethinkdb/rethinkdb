/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_AT_IMPL_05042005_0741)
#define FUSION_AT_IMPL_05042005_0741

#include <boost/fusion/support/detail/access.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/int.hpp>

namespace boost { namespace fusion
{
    struct vector_tag;

    namespace extension
    {
        template <typename Tag>
        struct at_impl;

        template <>
        struct at_impl<vector_tag>
        {
            template <typename Sequence, typename N>
            struct apply 
            {
                typedef typename mpl::at<typename Sequence::types, N>::type element;
                typedef typename detail::ref_result<element>::type type;
    
                static type
                call(Sequence& v)
                {
                    return v.at_impl(N());
                }
            };

            template <typename Sequence, typename N>
            struct apply <Sequence const, N>
            {
                typedef typename mpl::at<typename Sequence::types, N>::type element;
                typedef typename detail::cref_result<element>::type type;
    
                static type
                call(Sequence const& v)
                {
                    return v.at_impl(N());
                }
            };
        };
    }
}}

#endif
