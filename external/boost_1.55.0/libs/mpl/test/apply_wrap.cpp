
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: apply_wrap.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/apply_wrap.hpp>
#include <boost/mpl/limits/arity.hpp>
#include <boost/mpl/aux_/preprocessor/params.hpp>
#include <boost/mpl/aux_/preprocessor/enum.hpp>
#include <boost/mpl/aux_/test.hpp>

#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/dec.hpp>
#include <boost/preprocessor/if.hpp>
#include <boost/preprocessor/cat.hpp>

#if !defined(BOOST_MPL_CFG_NO_DEFAULT_PARAMETERS_IN_NESTED_TEMPLATES)
#   define APPLY_0_FUNC_DEF(i) \
    struct f0 \
    { \
        template< typename T = int > struct apply { typedef char type; }; \
    }; \
/**/
#else
#   define APPLY_0_FUNC_DEF(i) \
    struct f0 \
    { \
        template< typename T > struct apply { typedef char type; }; \
    }; \
/**/
#endif

#define APPLY_N_FUNC_DEF(i) \
    struct first##i \
    { \
        template< BOOST_MPL_PP_PARAMS(i, typename U) > \
        struct apply { typedef U1 type; }; \
    }; \
    \
    struct last##i \
    { \
        template< BOOST_MPL_PP_PARAMS(i, typename U) > \
        struct apply { typedef BOOST_PP_CAT(U,i) type; }; \
    }; \
/**/

#define APPLY_FUNC_DEF(z, i, unused) \
    BOOST_PP_IF( \
          i \
        , APPLY_N_FUNC_DEF \
        , APPLY_0_FUNC_DEF \
        )(i) \
/**/

namespace { namespace test {

BOOST_PP_REPEAT(
      BOOST_MPL_LIMIT_METAFUNCTION_ARITY
    , APPLY_FUNC_DEF
    , unused
    )

struct g0 { struct apply { typedef char type; }; };

}}

#define APPLY_0_TEST(i, apply_) \
    typedef apply_<test::f##i>::type t; \
    { MPL_ASSERT(( boost::is_same<t, char> )); } \
/**/

#define APPLY_N_TEST(i, apply_) \
    typedef apply_< \
          test::first##i \
        , char \
        BOOST_PP_COMMA_IF(BOOST_PP_DEC(i)) \
        BOOST_MPL_PP_ENUM(BOOST_PP_DEC(i), int) \
        >::type t1##i; \
    \
    typedef apply_< \
          test::last##i \
        , BOOST_MPL_PP_ENUM(BOOST_PP_DEC(i), int) \
        BOOST_PP_COMMA_IF(BOOST_PP_DEC(i)) char \
        >::type t2##i; \
    { MPL_ASSERT(( boost::is_same<t1##i, char> )); } \
    { MPL_ASSERT(( boost::is_same<t2##i, char> )); } \
/**/

#define APPLY_TEST(z, i, unused) \
    BOOST_PP_IF( \
          i \
        , APPLY_N_TEST \
        , APPLY_0_TEST \
        )(i, BOOST_PP_CAT(apply_wrap,i)) \
/**/


MPL_TEST_CASE()
{
    BOOST_PP_REPEAT(
          BOOST_MPL_LIMIT_METAFUNCTION_ARITY
        , APPLY_TEST
        , unused
        )

#if !defined(BOOST_MPL_CFG_NO_HAS_APPLY)
    {
        typedef apply_wrap0<test::g0>::type t;
        MPL_ASSERT(( boost::is_same<t, char> ));
    }
#endif
}
