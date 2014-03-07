/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2004 Daniel Wallin

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_SCOPE_DETAIL_LOCAL_VARIABLE_HPP
#define PHOENIX_SCOPE_DETAIL_LOCAL_VARIABLE_HPP

#include <boost/mpl/int.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/size.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/value_at.hpp>
#include <boost/preprocessor/enum.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_reference.hpp>

#define PHOENIX_MAP_LOCAL_TEMPLATE_PARAM(z, n, data) \
    typename T##n = unused<n>

#define PHOENIX_MAP_LOCAL_DISPATCH(z, n, data)  \
    typedef char(&result##n)[n+2];              \
    static result##n get(T##n*);

namespace boost { namespace phoenix
{
    template <typename Env, typename OuterEnv, typename Locals, typename Map>
    struct scoped_environment;

    namespace detail
    {
        template <typename Env>
        struct initialize_local
        {
            template <class F>
            struct result;

            template <class F, class Actor>
            struct result<F(Actor)>
            {
                typedef typename remove_reference<Actor>::type actor_type;
                typedef typename actor_type::template result<Env>::type type;
            };

            initialize_local(Env const& env)
                : env(env) {}

            template <typename Actor>
            typename result<initialize_local(Actor)>::type
            operator()(Actor const& actor) const
            {
                return actor.eval(env);
            }

            Env const& env;

        private:
            // silence MSVC warning C4512: assignment operator could not be generated
            initialize_local& operator= (initialize_local const&);
        };

        template <typename T>
        struct is_scoped_environment : mpl::false_ {};

        template <typename Env, typename OuterEnv, typename Locals, typename Map>
        struct is_scoped_environment<scoped_environment<Env, OuterEnv, Locals, Map> >
            : mpl::true_ {};

        template <int N>
        struct unused;

        template <BOOST_PP_ENUM(
            PHOENIX_LOCAL_LIMIT, PHOENIX_MAP_LOCAL_TEMPLATE_PARAM, _)>
        struct map_local_index_to_tuple
        {
            typedef char(&not_found)[1];
            static not_found get(...);

            BOOST_PP_REPEAT(PHOENIX_LOCAL_LIMIT, PHOENIX_MAP_LOCAL_DISPATCH, _)
        };

        template<typename T>
        T* generate_pointer();

        template <typename Map, typename Tag>
        struct get_index
        {
            BOOST_STATIC_CONSTANT(int,
                value = (
                    static_cast<int>((sizeof(Map::get(generate_pointer<Tag>()))) / sizeof(char)) - 2
                ));

            // if value == -1, Tag is not found
            typedef mpl::int_<value> type;
        };

        template <typename Local, typename Env>
        struct apply_local;

        template <typename Local, typename Env>
        struct outer_local
        {
            typedef typename
                apply_local<Local, typename Env::outer_env_type>::type
            type;
        };

        template <typename Locals, typename Index>
        struct get_local_or_void
        {
            typedef typename
                mpl::eval_if<
                    mpl::less<Index, mpl::size<Locals> >
                  , fusion::result_of::at<Locals, Index>
                  , mpl::identity<fusion::void_>
                >::type
            type;
        };

        template <typename Local, typename Env, typename Index>
        struct get_local_from_index
        {
            typedef typename
                mpl::eval_if<
                    mpl::equal_to<Index, mpl::int_<-1> >
                  , outer_local<Local, Env>
                  , get_local_or_void<typename Env::locals_type, Index>
                >::type
            type;
        };

        template <typename Local, typename Env>
        struct get_local
        {
            typedef typename
                get_index<
                    typename Env::map_type, typename Local::key_type>::type
            index_type;

            typedef typename
                get_local_from_index<Local, Env, index_type>::type
            type;
        };

        template <typename Local, typename Env>
        struct apply_local
        {
            // $$$ TODO: static assert that Env is a scoped_environment $$$
            typedef typename get_local<Local, Env>::type type;
        };

        template <typename Key>
        struct eval_local
        {
            template <typename RT, typename Env, typename Index>
            static RT
            get(Env const& env, Index, mpl::false_)
            {
                return RT(fusion::at<Index>(env.locals));
            }

            template <typename RT, typename Env, typename Index>
            static RT
            get(Env const& env, Index index, mpl::true_)
            {
                typedef typename
                    get_index<typename Env::outer_env_type::map_type, Key>::type
                index_type;

                return get<RT>(
                    env.outer_env
                  , index_type()
                  , mpl::equal_to<index_type, mpl::int_<-1> >());
            }

            template <typename RT, typename Env, typename Index>
            static RT
            get(Env const& env, Index index)
            {
                return get<RT>(
                    env
                  , index
                  , mpl::equal_to<Index, mpl::int_<-1> >());
            }
        };
    }
}}

#undef PHOENIX_MAP_LOCAL_TEMPLATE_PARAM
#undef PHOENIX_MAP_LOCAL_DISPATCH
#endif
