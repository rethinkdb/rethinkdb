/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2004 Daniel Wallin

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_SCOPE_LAMBDA_HPP
#define PHOENIX_SCOPE_LAMBDA_HPP

#include <boost/spirit/home/phoenix/core/limits.hpp>
#include <boost/spirit/home/phoenix/core/composite.hpp>
#include <boost/spirit/home/phoenix/scope/scoped_environment.hpp>
#include <boost/spirit/home/phoenix/scope/detail/local_variable.hpp>
#include <boost/spirit/home/phoenix/detail/local_reference.hpp>
#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/fusion/include/transform.hpp>
#include <boost/fusion/include/as_vector.hpp>

namespace boost { namespace phoenix
{
    template <typename Base, typename OuterEnv, typename Locals, typename Map>
    struct lambda_eval : Base
    {
        template <typename Env>
        struct result
        {
            typedef typename Base::template
                result<scoped_environment<Env, OuterEnv, Locals, Map> >::type
            result_type;

            typedef typename 
                detail::unwrap_local_reference<result_type>::type 
            type;
        };

        lambda_eval(
            Base const& base
          , OuterEnv const& outer_env
          , Locals const& locals)
            : Base(base)
            , outer_env(outer_env)
            , locals(locals) {}

        template <typename Env>
        typename result<Env>::type
        eval(Env const& env) const
        {
            typedef typename result<Env>::type RT;
            return RT(Base::eval(
                scoped_environment<Env, OuterEnv, Locals, Map>(
                    env, outer_env, locals)));
        }

        OuterEnv outer_env;
        mutable Locals locals;
    };
    
    template <typename Base, typename Vars, typename Map>
    struct lambda_actor
    {
        typedef typename
            mpl::fold<
                Vars
              , mpl::false_
              , detail::compute_no_nullary
            >::type
        no_nullary;

        template <typename Env>
        struct result
        {
            typedef typename 
                fusion::result_of::as_vector<
                    typename fusion::result_of::transform<
                        Vars
                      , detail::initialize_local<Env>
                    >::type
                >::type 
            locals_type;

            typedef actor<lambda_eval<Base, Env, locals_type, Map> > type;
        };

        lambda_actor(Base const& f, Vars const& vars)
            : f(f), vars(vars) {}

        template <typename Env>
        typename result<Env>::type
        eval(Env const& env) const
        {
            typedef typename result<Env>::type result_type;
            
            return result_type(
                f, env, fusion::as_vector(
                    fusion::transform(
                        vars
                      , detail::initialize_local<Env>(env)
                    )));
        }

        Base f;
        Vars vars;
    };
    
    template <typename Vars, typename Map>
    struct lambda_actor_gen
    {
        template <typename Base>
        actor<lambda_actor<Base, Vars, Map> > const
        operator[](actor<Base> const& f) const
        {
            return lambda_actor<Base, Vars, Map>(f, vars);
        }

        lambda_actor_gen(Vars const& vars)
            : vars(vars) {}

        Vars vars;
    };

    template <typename Key>
    struct local_variable; // forward
    struct assign_eval; // forward

    struct lambda_gen 
        : lambda_actor_gen<
            fusion::vector<>
          , detail::map_local_index_to_tuple<> >
    {
        typedef 
            lambda_actor_gen<
                fusion::vector<>
              , detail::map_local_index_to_tuple<> >
        base_type;

        lambda_gen()
            : base_type(fusion::vector<>())
        {
        }

        template <typename K0, typename V0>
        lambda_actor_gen<
            fusion::vector<V0>
          , detail::map_local_index_to_tuple<K0>
        >
        operator()(
            actor<composite<assign_eval, fusion::vector<local_variable<K0>, V0> > > const& a0
        ) const
        {
            return fusion::vector<V0>(fusion::at_c<1>(a0));
        }
    
        template <typename K0, typename K1, typename V0, typename V1>
        lambda_actor_gen<
            fusion::vector<V0, V1>
          , detail::map_local_index_to_tuple<K0, K1>
        >
        operator()(
            actor<composite<assign_eval, fusion::vector<local_variable<K0>, V0> > > const& a0
          , actor<composite<assign_eval, fusion::vector<local_variable<K1>, V1> > > const& a1
        ) const
        {
            return fusion::vector<V0, V1>(fusion::at_c<1>(a0), fusion::at_c<1>(a1));
        }
        
        // Bring in the rest...
        #define PHOENIX_LOCAL_GEN_NAME lambda_actor_gen
        #include <boost/spirit/home/phoenix/scope/detail/local_gen.hpp>
        #undef PHOENIX_LOCAL_GEN_NAME
    };

    lambda_gen const lambda = lambda_gen();
}}

#endif
