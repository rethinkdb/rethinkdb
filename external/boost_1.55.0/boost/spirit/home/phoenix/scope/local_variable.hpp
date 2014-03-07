/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2004 Daniel Wallin

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_SCOPE_LOCAL_VARIABLE_HPP
#define PHOENIX_SCOPE_LOCAL_VARIABLE_HPP

#include <boost/spirit/home/phoenix/core/limits.hpp>
#include <boost/spirit/home/phoenix/detail/local_reference.hpp>
#include <boost/spirit/home/phoenix/scope/detail/local_variable.hpp>
#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/mpl/bool.hpp>

namespace boost { namespace phoenix
{
    template <typename Key>
    struct local_variable
    {
        typedef Key key_type;

        // This will prevent actor::operator()() from kicking in.
        // Actually, we do not need all actor::operator()s for
        // all arities, but this will suffice. The nullary 
        // actor::operator() is particularly troublesome because 
        // it is always eagerly evaluated by the compiler.
        typedef mpl::true_ no_nullary; 

        template <typename Env>
        struct result : detail::apply_local<local_variable<Key>, Env> {};

        template <typename Env>
        typename result<Env>::type 
        eval(Env const& env) const
        {
            typedef typename result<Env>::type return_type;
            typedef typename 
                detail::get_index<typename Env::map_type, Key>::type 
            index_type;
            typedef detail::eval_local<Key> eval_local;

            return eval_local::template get<return_type>(
                env
              , index_type());
        }

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        local_variable& operator= (local_variable const&);
    };

    namespace local_names
    {
        actor<local_variable<struct _a_key> > const _a 
            = local_variable<struct _a_key>();
        actor<local_variable<struct _b_key> > const _b 
            = local_variable<struct _b_key>();
        actor<local_variable<struct _c_key> > const _c 
            = local_variable<struct _c_key>();
        actor<local_variable<struct _d_key> > const _d 
            = local_variable<struct _d_key>();
        actor<local_variable<struct _e_key> > const _e 
            = local_variable<struct _e_key>();
        actor<local_variable<struct _f_key> > const _f 
            = local_variable<struct _f_key>();
        actor<local_variable<struct _g_key> > const _g 
            = local_variable<struct _g_key>();
        actor<local_variable<struct _h_key> > const _h 
            = local_variable<struct _h_key>();
        actor<local_variable<struct _i_key> > const _i 
            = local_variable<struct _i_key>();
        actor<local_variable<struct _j_key> > const _j 
            = local_variable<struct _j_key>();
        actor<local_variable<struct _k_key> > const _k 
            = local_variable<struct _k_key>();
        actor<local_variable<struct _l_key> > const _l 
            = local_variable<struct _l_key>();
        actor<local_variable<struct _m_key> > const _m 
            = local_variable<struct _m_key>();
        actor<local_variable<struct _n_key> > const _n 
            = local_variable<struct _n_key>();
        actor<local_variable<struct _o_key> > const _o 
            = local_variable<struct _o_key>();
        actor<local_variable<struct _p_key> > const _p 
            = local_variable<struct _p_key>();
        actor<local_variable<struct _q_key> > const _q 
            = local_variable<struct _q_key>();
        actor<local_variable<struct _r_key> > const _r 
            = local_variable<struct _r_key>();
        actor<local_variable<struct _s_key> > const _s 
            = local_variable<struct _s_key>();
        actor<local_variable<struct _t_key> > const _t 
            = local_variable<struct _t_key>();
        actor<local_variable<struct _u_key> > const _u 
            = local_variable<struct _u_key>();
        actor<local_variable<struct _v_key> > const _v 
            = local_variable<struct _v_key>();
        actor<local_variable<struct _w_key> > const _w 
            = local_variable<struct _w_key>();
        actor<local_variable<struct _x_key> > const _x 
            = local_variable<struct _x_key>();
        actor<local_variable<struct _y_key> > const _y 
            = local_variable<struct _y_key>();
        actor<local_variable<struct _z_key> > const _z 
            = local_variable<struct _z_key>();
    }
}}

#endif
