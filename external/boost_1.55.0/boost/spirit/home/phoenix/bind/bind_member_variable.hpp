/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_BIND_BIND_MEMBER_VARIABLE_HPP
#define PHOENIX_BIND_BIND_MEMBER_VARIABLE_HPP

#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>
#include <boost/spirit/home/phoenix/core/reference.hpp>
#include <boost/spirit/home/phoenix/core/detail/function_eval.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace boost { namespace phoenix
{
    namespace detail
    {
        template <typename RT, typename MP>
        struct member_variable
        {
            template <typename Class>
            struct result
            {
                typedef typename boost::mpl::if_<
                    boost::is_const<
                        typename boost::remove_pointer<
                            typename boost::remove_reference<Class>::type
                        >::type
                    >
                  , const RT&
                  , RT&
                >::type type;
            };

            member_variable(MP mp)
                : mp(mp) {}

            template <typename Class>
            RT& operator()(Class& obj) const
            {
                return obj.*mp;
            }

            template <typename Class>
            RT& operator()(Class* obj) const
            {
                return obj->*mp;
            }

            template <typename Class>
            RT const& operator()(Class const& obj) const
            {
                return obj.*mp;
            }

            template <typename Class>
            RT const& operator()(Class const* obj) const
            {
                return obj->*mp;
            }

            MP mp;
        };
    }

    template <typename RT, typename ClassT, typename ClassA>
    inline actor<
        typename as_composite<
            detail::function_eval<1>
          , detail::member_variable<RT, RT ClassT::*>
          , ClassA
        >::type>
    bind(RT ClassT::*mp, ClassA const& obj)
    {
        typedef detail::member_variable<RT, RT ClassT::*> mp_type;
        return compose<detail::function_eval<1> >(mp_type(mp), obj);
    }

    template <typename RT, typename ClassT>
    inline actor<
        typename as_composite<
            detail::function_eval<1>
          , detail::member_variable<RT, RT ClassT::*>
          , actor<reference<ClassT> >
        >::type>
    bind(RT ClassT::*mp, ClassT& obj)
    {
        typedef detail::member_variable<RT, RT ClassT::*> mp_type;
        return compose<detail::function_eval<1> >(
            mp_type(mp)
          , actor<reference<ClassT> >(reference<ClassT>(obj)));
    }
}}

#endif
