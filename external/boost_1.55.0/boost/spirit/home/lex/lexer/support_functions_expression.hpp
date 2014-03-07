//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c)      2011 Thomas Heller
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(SPIRIT_LEX_SUPPORT_FUNCTIONS_EXPRESSION_MAR_22_2011_0711PM)
#define SPIRIT_LEX_SUPPORT_FUNCTIONS_EXPRESSION_MAR_22_2011_0711PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/include/phoenix_core.hpp>

namespace boost { namespace spirit { namespace lex
{
    template <typename> struct less_type;
    struct more_type;
    template <typename, typename> struct lookahead_type;
}}}

///////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_SPIRIT_USE_PHOENIX_V3

namespace boost { namespace spirit { namespace lex
{
    namespace expression
    {
        template <typename Eval>
        struct less
        {
            typedef phoenix::actor<lex::less_type<Eval> > type;

            static type make(Eval const & eval)
            {
                return lex::less_type<Eval>(eval);
            }
        };

        template <typename IdType, typename State>
        struct lookahead
        {
            typedef phoenix::actor<lex::lookahead_type<IdType, State> > type;

            static type make(IdType const & id_type, State const & state)
            {
                return lex::lookahead_type<IdType, State>(id_type, state);
            }
        };
    }
}}}

#else // BOOST_SPIRIT_USE_PHOENIX_V3

BOOST_PHOENIX_DEFINE_EXPRESSION(
    (boost)(spirit)(lex)(less)
  , (boost::phoenix::meta_grammar)
)

BOOST_PHOENIX_DEFINE_EXPRESSION(
    (boost)(spirit)(lex)(lookahead)
  , (boost::phoenix::meta_grammar)
    (boost::phoenix::meta_grammar)
)

namespace boost { namespace phoenix
{

    namespace result_of
    {
        template <>
        struct is_nullary<custom_terminal<boost::spirit::lex::more_type> >
          : mpl::false_
        {};
    }
    
    template <typename Dummy>
    struct is_custom_terminal<boost::spirit::lex::more_type, Dummy> : mpl::true_ {};
    
    template <typename Dummy>
    struct custom_terminal<boost::spirit::lex::more_type, Dummy>
        : proto::call<
            v2_eval(
                proto::make<boost::spirit::lex::more_type()>
              , proto::call<functional::env(proto::_state)>
            )
        >
    {};


    template <typename Dummy>
    struct is_nullary::when<spirit::lex::rule::less, Dummy>
      : proto::make<mpl::false_()>
    {};

    template <typename Dummy>
    struct default_actions::when<spirit::lex::rule::less, Dummy>
      : proto::call<
            v2_eval(
                proto::make<
                    spirit::lex::less_type<proto::_child0>(proto::_child0)
                >
              , _env
            )
        >
    {};

    template <typename Dummy>
    struct is_nullary::when<spirit::lex::rule::lookahead, Dummy>
      : proto::make<mpl::false_()>
    {};

    template <typename Dummy>
    struct default_actions::when<spirit::lex::rule::lookahead, Dummy>
      : proto::call<
            v2_eval(
                proto::make<
                    spirit::lex::lookahead_type<
                        proto::_child0
                      , proto::_child1
                    >(
                        proto::_child0
                      , proto::_child1
                    )
                >
              , _env
            )
        >
    {};
}}

#endif // BOOST_SPIRIT_USE_PHOENIX_V3

#endif
