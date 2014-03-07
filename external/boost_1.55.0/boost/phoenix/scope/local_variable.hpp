/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010-2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_SCOPE_LOCAL_VARIABLE_HPP
#define BOOST_PHOENIX_SCOPE_LOCAL_VARIABLE_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/core/call.hpp>
#include <boost/phoenix/core/expression.hpp>
#include <boost/phoenix/core/reference.hpp>
#include <boost/phoenix/core/value.hpp>
#include <boost/phoenix/scope/scoped_environment.hpp>
#include <boost/phoenix/scope/detail/local_variable.hpp>
#include <boost/phoenix/statement/sequence.hpp>

namespace boost { namespace phoenix
{
    namespace detail
    {
    }
    
    namespace expression
    {
        template <typename Key>
        struct local_variable
            : expression::terminal<detail::local<Key> >
        {
            typedef typename expression::terminal<detail::local<Key> >::type type;

            static type make()
            {
                type const e = {};
                return e;
            }
        };
    }

    namespace rule
    {
        struct local_variable
            : expression::local_variable<proto::_>
        {};

        struct local_var_def
            : proto::assign<local_variable, meta_grammar>
        {};
    }

    namespace result_of
    {
        template <typename Key>
        struct is_nullary<custom_terminal<detail::local<Key> > >
            : mpl::false_
        {};
    }

    namespace detail
    {
        struct scope_is_nullary_actions
        {
            template <typename Rule, typename Dummy = void>
            struct when
                : boost::phoenix::is_nullary::when<Rule, Dummy>
            {};
        };

        template <typename Dummy>
        struct scope_is_nullary_actions::when<boost::phoenix::rule::custom_terminal, Dummy>
            : proto::or_<
                proto::when<boost::phoenix::rule::local_variable, mpl::true_()>
              , proto::otherwise<
                    is_nullary::when<boost::phoenix::rule::custom_terminal, Dummy>
                >
            >
        {};

        struct local_var_not_found
        {
        };
    }
    
    template<typename Key>
    struct is_custom_terminal<detail::local<Key> >
      : mpl::true_
    {};

    template <typename Key>
    struct custom_terminal<detail::local<Key> >
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Local, typename Context>
        struct result<This(Local, Context)>
            : result<This(Local const &, Context)>
        {};

        template <typename This, typename Local, typename Context>
        struct result<This(Local &, Context)>
        {
            typedef
                typename remove_reference<
                    typename result_of::env<Context>::type
                >::type
                env_type;
                
                typedef typename detail::apply_local<detail::local<Key>, env_type>::type type;
        };

        template <typename Local, typename Context>
        typename result<custom_terminal(Local const &, Context const&)>::type
        operator()(Local, Context const & ctx)
        {
            typedef
                typename remove_reference<
                    typename result_of::env<Context>::type
                >::type
                env_type;
                
                typedef typename detail::apply_local<detail::local<Key>, env_type>::type return_type;
            
            static const int index_value = detail::get_index<typename env_type::map_type, detail::local<Key> >::value;

            typedef detail::eval_local<Key> eval_local;

            return eval_local::template get<return_type, index_value>(
                phoenix::env(ctx));
        }
    };

    namespace local_names
    {
        typedef expression::local_variable<struct _a_key>::type _a_type;
        typedef expression::local_variable<struct _b_key>::type _b_type;
        typedef expression::local_variable<struct _c_key>::type _c_type;
        typedef expression::local_variable<struct _d_key>::type _d_type;
        typedef expression::local_variable<struct _e_key>::type _e_type;
        typedef expression::local_variable<struct _f_key>::type _f_type;
        typedef expression::local_variable<struct _g_key>::type _g_type;
        typedef expression::local_variable<struct _h_key>::type _h_type;
        typedef expression::local_variable<struct _i_key>::type _i_type;
        typedef expression::local_variable<struct _j_key>::type _j_type;
        typedef expression::local_variable<struct _k_key>::type _k_type;
        typedef expression::local_variable<struct _l_key>::type _l_type;
        typedef expression::local_variable<struct _m_key>::type _m_type;
        typedef expression::local_variable<struct _n_key>::type _n_type;
        typedef expression::local_variable<struct _o_key>::type _o_type;
        typedef expression::local_variable<struct _p_key>::type _p_type;
        typedef expression::local_variable<struct _q_key>::type _q_type;
        typedef expression::local_variable<struct _r_key>::type _r_type;
        typedef expression::local_variable<struct _s_key>::type _s_type;
        typedef expression::local_variable<struct _t_key>::type _t_type;
        typedef expression::local_variable<struct _u_key>::type _u_type;
        typedef expression::local_variable<struct _v_key>::type _v_type;
        typedef expression::local_variable<struct _w_key>::type _w_type;
        typedef expression::local_variable<struct _x_key>::type _x_type;
        typedef expression::local_variable<struct _y_key>::type _y_type;
        typedef expression::local_variable<struct _z_key>::type _z_type;

#ifndef BOOST_PHOENIX_NO_PREDEFINED_TERMINALS
        _a_type const _a = {{{}}};
        _b_type const _b = {{{}}};
        _c_type const _c = {{{}}};
        _d_type const _d = {{{}}};
        _e_type const _e = {{{}}};
        _f_type const _f = {{{}}};
        _g_type const _g = {{{}}};
        _h_type const _h = {{{}}};
        _i_type const _i = {{{}}};
        _j_type const _j = {{{}}};
        _k_type const _k = {{{}}};
        _l_type const _l = {{{}}};
        _m_type const _m = {{{}}};
        _n_type const _n = {{{}}};
        _o_type const _o = {{{}}};
        _p_type const _p = {{{}}};
        _q_type const _q = {{{}}};
        _r_type const _r = {{{}}};
        _s_type const _s = {{{}}};
        _t_type const _t = {{{}}};
        _u_type const _u = {{{}}};
        _v_type const _v = {{{}}};
        _w_type const _w = {{{}}};
        _x_type const _x = {{{}}};
        _y_type const _y = {{{}}};
        _z_type const _z = {{{}}};
#endif
    }
}}

#endif
