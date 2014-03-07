/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_DEREF_IMPL_20060222_1952)
#define BOOST_FUSION_DEREF_IMPL_20060222_1952

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/mpl/if.hpp>

#include <string>

namespace example
{
    struct example_struct_iterator_tag;

    template<typename Struct, int Pos>
    struct example_struct_iterator;
}

namespace boost { namespace fusion {

    namespace extension
    {
        template<typename Tag>
        struct deref_impl;

        template<>
        struct deref_impl<example::example_struct_iterator_tag>
        {
            template<typename Iterator>
            struct apply;

            template<typename Struct>
            struct apply<example::example_struct_iterator<Struct, 0> >
            {
                typedef typename mpl::if_<
                    is_const<Struct>, std::string const&, std::string&>::type type;

                static type
                call(example::example_struct_iterator<Struct, 0> const& it)
                {
                    return it.struct_.name;
                }
            };

            template<typename Struct>
            struct apply<example::example_struct_iterator<Struct, 1> >
            {
                typedef typename mpl::if_<
                    is_const<Struct>, int const&, int&>::type type;

                static type
                call(example::example_struct_iterator<Struct, 1> const& it)
                {
                    return it.struct_.age;
                }
            };
        };
    }
}}

#endif
