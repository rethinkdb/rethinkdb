/*=============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef PHOENIX_OPERATOR_MEMBER_HPP
#define PHOENIX_OPERATOR_MEMBER_HPP

#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/spirit/home/phoenix/core/composite.hpp>
#include <boost/spirit/home/phoenix/core/compose.hpp>

#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_member_pointer.hpp>
#include <boost/type_traits/is_member_function_pointer.hpp>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>

#include <boost/utility/enable_if.hpp>

#include <boost/get_pointer.hpp>

#include <boost/spirit/home/phoenix/operator/detail/mem_fun_ptr_gen.hpp>

#include <memory>

namespace boost { 
    template<typename T> class shared_ptr;
    template<typename T> class scoped_ptr;

namespace phoenix {
    namespace detail
    {
        template<typename T>
        struct member_type;
        
        template<typename Class, typename MemberType>
        struct member_type<MemberType (Class::*)>
        {
            typedef MemberType type;
        };
    }

    namespace meta
    {
        template<typename T> 
        struct pointed_type;

        template<typename T>
        struct pointed_type<T*>
        {
            typedef T type;
        };

        template<typename T>
        struct pointed_type<shared_ptr<T> >
        {
            typedef T type;
        };
        
        template<typename T>
        struct pointed_type<scoped_ptr<T> >
        {
            typedef T type;
        };

        template<typename T>
        struct pointed_type<std::auto_ptr<T> >
        {
            typedef T type;
        };
    }

    struct member_object_eval
    {
        template<typename Env, typename PtrActor, typename MemPtrActor>
        struct result
        {
            typedef typename detail::member_type<
                typename eval_result<MemPtrActor, Env>::type>::type member_type;

            typedef typename meta::pointed_type<
                typename remove_reference<
                typename eval_result<PtrActor, Env>::type>::type>::type object_type;

            typedef typename add_reference<
                typename mpl::eval_if<
                is_const<object_type>,
                add_const<member_type>,
                mpl::identity<member_type> >::type>::type type;
        };

        template<typename Rt, typename Env, typename PtrActor, typename MemPtrActor>
        static typename result<Env,PtrActor,MemPtrActor>::type
        eval(const Env& env, PtrActor& ptrActor, MemPtrActor& memPtrActor)
        {
            return get_pointer(ptrActor.eval(env))->*memPtrActor.eval(env);
        }
    };

    namespace member_object
    {
        template<typename T0, typename MemObjPtr>
        typename enable_if<
            mpl::and_<is_member_pointer<MemObjPtr>, mpl::not_<is_member_function_pointer<MemObjPtr> > >,
            actor<typename as_composite<
            member_object_eval, actor<T0>,
            typename as_actor<MemObjPtr>::type>::type> >::type
        operator->*(
            const actor<T0>& ptrActor, 
            MemObjPtr memObjPtr)
        {
            return compose<member_object_eval>(
                ptrActor,
                as_actor<MemObjPtr>::convert(memObjPtr));
        }
    }

    namespace member_function
    {
        template<typename T0, typename MemFunPtr>
        typename enable_if<
            is_member_function_pointer<MemFunPtr>,
            mem_fun_ptr_gen<actor<T0>, MemFunPtr> >::type
        operator->*(const actor<T0>& ptrActor, MemFunPtr memFunPtr)
        {
            return mem_fun_ptr_gen<actor<T0>, MemFunPtr>(
                ptrActor, memFunPtr);
        }
    }

    using member_object::operator->*;
    using member_function::operator->*;
}}

#endif
