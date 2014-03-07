/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2004 Daniel Wallin

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_SCOPE_DYNAMIC_HPP
#define PHOENIX_SCOPE_DYNAMIC_HPP

#include <boost/spirit/home/phoenix/core/limits.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/spirit/home/phoenix/core/actor.hpp>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/inc.hpp>

#include <boost/noncopyable.hpp>
#include <boost/assert.hpp>

#define PHOENIX_DYNAMIC_MEMBER(z, n, data)                                      \
    typedef actor<dynamic_member<n, self_type> >                                \
        BOOST_PP_CAT(member, BOOST_PP_INC(n));

namespace boost { namespace phoenix
{
    template <typename DynamicScope>
    struct dynamic_frame : noncopyable
    {
        typedef typename DynamicScope::tuple_type tuple_type;

        dynamic_frame(DynamicScope const& scope)
            : tuple()
            , save(scope.frame)
            , scope(scope)
        {
            scope.frame = this;
        }

        template <typename Tuple>
        dynamic_frame(DynamicScope const& scope, Tuple const& init)
            : tuple(init)
            , save(scope.frame)
            , scope(scope)
        {
            scope.frame = this;
        }

        ~dynamic_frame()
        {
            scope.frame = save;
        }

        tuple_type& data() { return tuple; }
        tuple_type const& data() const { return tuple; }

    private:

        tuple_type tuple;
        dynamic_frame* save;
        DynamicScope const& scope;
    };

    template <int N, typename DynamicScope>
    struct dynamic_member
    {
        typedef mpl::false_ no_nullary;
        typedef typename DynamicScope::tuple_type tuple_type;

        dynamic_member(DynamicScope const& scope)
            : scope(scope) {}

        template <typename Env>
        struct result
        {
            typedef typename
                fusion::result_of::at_c<tuple_type, N>::type
            type;
        };

        template <typename Env>
        typename result<Env>::type
        eval(Env const& /*env*/) const
        {
            BOOST_ASSERT(scope.frame != 0);
            return fusion::at_c<N>(scope.frame->data());
        }

    private:

        DynamicScope const& scope;
    };

    template <BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(PHOENIX_DYNAMIC_LIMIT, typename T, void_)>
    struct dynamic : noncopyable
    {
        typedef fusion::vector<BOOST_PP_ENUM_PARAMS(PHOENIX_DYNAMIC_LIMIT, T)> tuple_type;
        typedef dynamic<BOOST_PP_ENUM_PARAMS(PHOENIX_DYNAMIC_LIMIT, T)> self_type;
        typedef dynamic_frame<self_type> dynamic_frame_type;

        dynamic()
            : frame(0) {}

        BOOST_PP_REPEAT(PHOENIX_DYNAMIC_LIMIT, PHOENIX_DYNAMIC_MEMBER, _)

    private:

        template <int N, typename DynamicScope>
        friend struct dynamic_member;

        template <typename DynamicScope>
        friend struct dynamic_frame;

        mutable dynamic_frame_type* frame;
    };
}}

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355)
#endif

/*
    PHOENIX_DYNAMIC macro(name, type-name sequence)
    Example:

    PHOENIX_DYNAMIC(
        my_dynamic,
            (int, num)
            (std::string, message)
            (double, real)
    );

    which expands to:

    struct my_dynamic : ::boost::phoenix::dynamic<int, std::string, double>
    {
        my_dynamic() : num(*this), message(*this), real(*this) {}

        member1 num;
        member2 message;
        member3 real;
    };

    PHOENIX_DYNAMIC takes the input (containing a binary sequence)
    and converts that sequence to a unary sequence of
    binary tuples and passes it on to PHOENIX_DYNAMIC_I.

    Thanks to Paul Mensonides for the PP macro help
*/

#define PHOENIX_DYNAMIC(name, bseq)                                             \
    PHOENIX_DYNAMIC_I(name, BOOST_PP_CAT(PHOENIX_DYNAMIC_X bseq, 0))            \

#define PHOENIX_DYNAMIC_X(x, y) ((x, y)) PHOENIX_DYNAMIC_Y
#define PHOENIX_DYNAMIC_Y(x, y) ((x, y)) PHOENIX_DYNAMIC_X
#define PHOENIX_DYNAMIC_X0
#define PHOENIX_DYNAMIC_Y0

// PHOENIX_DYNAMIC_I generates the overarching structure and uses
// SEQ_FOR_EACH_I to generate the "linear" substructures.

#define PHOENIX_DYNAMIC_I(name, seq)                                            \
    struct name :                                                               \
        ::boost::phoenix::dynamic<                                              \
            BOOST_PP_SEQ_FOR_EACH_I(PHOENIX_DYNAMIC_A, ~, seq)> {               \
        name() : BOOST_PP_SEQ_FOR_EACH_I(PHOENIX_DYNAMIC_B, ~, seq) {}          \
        BOOST_PP_SEQ_FOR_EACH_I(PHOENIX_DYNAMIC_C, ~, seq)                      \
    }                                                                           \

#define PHOENIX_DYNAMIC_A(r, _, i, xy)                                          \
    BOOST_PP_COMMA_IF(i) BOOST_PP_TUPLE_ELEM(2, 0, xy)                          \

#define PHOENIX_DYNAMIC_B(r, _, i, xy)                                          \
    BOOST_PP_COMMA_IF(i) BOOST_PP_TUPLE_ELEM(2, 1, xy)(*this)                   \

#define PHOENIX_DYNAMIC_C(r, _, i, xy)                                          \
    BOOST_PP_CAT(member, BOOST_PP_INC(i)) BOOST_PP_TUPLE_ELEM(2, 1, xy);        \

#undef PHOENIX_DYNAMIC_MEMBER

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#endif
