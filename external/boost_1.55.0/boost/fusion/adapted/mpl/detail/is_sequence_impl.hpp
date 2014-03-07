/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_IS_SEQUENCE_IMPL_31122005_1505)
#define BOOST_FUSION_IS_SEQUENCE_IMPL_31122005_1505

#include <boost/mpl/bool.hpp>

namespace boost { namespace fusion
{
    struct mpl_sequence_tag;

    namespace extension
    {
        template<typename Tag>
        struct is_sequence_impl;

        template<>
        struct is_sequence_impl<mpl_sequence_tag>
        {
            template<typename T>
            struct apply : mpl::true_ {};
        };
    }
}}

#endif
