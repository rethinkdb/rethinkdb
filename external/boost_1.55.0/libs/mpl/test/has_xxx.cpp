
// Copyright Aleksey Gurtovoy 2000-2004
// Copyright Daniel Walker 2007
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: has_xxx.cpp 63907 2010-07-12 01:36:38Z djwalker $
// $Date: 2010-07-11 18:36:38 -0700 (Sun, 11 Jul 2010) $
// $Revision: 63907 $

#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/aux_/config/workaround.hpp>
#include <boost/mpl/aux_/test.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(xxx)
BOOST_MPL_HAS_XXX_TEMPLATE_NAMED_DEF(has_xxx_template, xxx, false)
BOOST_MPL_HAS_XXX_TEMPLATE_DEF(yyy)

struct a1 {};
struct a2 { void xxx(); };
struct a3 { int xxx; };
struct a4 { static int xxx(); };
struct a5 { template< typename T > struct xxx {}; };

struct b1 { typedef int xxx; };
struct b2 { struct xxx; };
struct b3 { typedef int& xxx; };
struct b4 { typedef int* xxx; };
struct b5 { typedef int xxx[10]; };
struct b6 { typedef void (*xxx)(); };
struct b7 { typedef void (xxx)(); };

struct c1 { template< typename T > struct xxx {}; };
struct c2 { template< typename T1, typename T2 > struct xxx {}; };
struct c3 { template< typename T1, typename T2, typename T3 > struct xxx {}; };
struct c4 { template< typename T1, typename T2, typename T3, typename T4 > struct xxx {}; };
struct c5 { template< typename T1, typename T2, typename T3, typename T4, typename T5 > struct xxx {}; };
struct c6 { template< typename T > struct yyy {}; };
struct c7 { template< typename T1, typename T2 > struct yyy {}; };

template< typename T > struct outer;
template< typename T > struct inner { typedef typename T::type type; };

// agurt, 15/aug/04: make sure MWCW passes the test in presence of the following
// template
template< typename T > struct xxx;


MPL_TEST_CASE()
{
    MPL_ASSERT_NOT(( has_xxx<int> ));
    MPL_ASSERT_NOT(( has_xxx_template<int> ));

#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
    MPL_ASSERT_NOT(( has_xxx<int&> ));
    MPL_ASSERT_NOT(( has_xxx_template<int&> ));

    MPL_ASSERT_NOT(( has_xxx<int*> ));
    MPL_ASSERT_NOT(( has_xxx_template<int*> ));

    MPL_ASSERT_NOT(( has_xxx<int[]> ));
    MPL_ASSERT_NOT(( has_xxx_template<int[]> ));

    MPL_ASSERT_NOT(( has_xxx<int (*)()> ));
    MPL_ASSERT_NOT(( has_xxx_template<int (*)()> ));

    MPL_ASSERT_NOT(( has_xxx<a2> ));
    MPL_ASSERT_NOT(( has_xxx_template<a2> ));

    MPL_ASSERT_NOT(( has_xxx<a3> ));
    MPL_ASSERT_NOT(( has_xxx_template<a3> ));

    MPL_ASSERT_NOT(( has_xxx<a4> ));
    MPL_ASSERT_NOT(( has_xxx_template<a4> ));

#if !BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3202))
    MPL_ASSERT_NOT(( has_xxx<a5> ));
    MPL_ASSERT(( has_xxx_template<a5> ));
#endif
    MPL_ASSERT_NOT(( has_xxx< enum_ > ));
    MPL_ASSERT_NOT(( has_xxx_template< enum_ > ));
#endif

    MPL_ASSERT_NOT(( has_xxx<a1> ));
    MPL_ASSERT_NOT(( has_xxx_template<a1> ));

    MPL_ASSERT_NOT(( has_xxx< outer< inner<int> > > ));
    MPL_ASSERT_NOT(( has_xxx_template< outer< inner<int> > > ));

    MPL_ASSERT_NOT(( has_xxx< incomplete > ));
    MPL_ASSERT_NOT(( has_xxx_template< incomplete > ));

    MPL_ASSERT_NOT(( has_xxx< abstract > ));
    MPL_ASSERT_NOT(( has_xxx_template< abstract > ));

    MPL_ASSERT_NOT(( has_xxx< noncopyable > ));
    MPL_ASSERT_NOT(( has_xxx_template< noncopyable > ));

#if !BOOST_WORKAROUND(__COMO_VERSION__, BOOST_TESTED_AT(4308))
    MPL_ASSERT_NOT(( has_xxx_template<b1> ));
    MPL_ASSERT_NOT(( has_xxx_template<b2> ));
    MPL_ASSERT_NOT(( has_xxx_template<b3> ));
    MPL_ASSERT_NOT(( has_xxx_template<b4> ));
    MPL_ASSERT_NOT(( has_xxx_template<b5> ));
    MPL_ASSERT_NOT(( has_xxx_template<b6> ));
    MPL_ASSERT_NOT(( has_xxx_template<b7> ));
#endif

    // Same name, different args.
    MPL_ASSERT(( has_xxx_template<c1> ));
    MPL_ASSERT(( has_xxx_template<c2> ));
    MPL_ASSERT(( has_xxx_template<c3> ));
    MPL_ASSERT(( has_xxx_template<c4> ));
    MPL_ASSERT(( has_xxx_template<c5> ));
    MPL_ASSERT(( has_yyy<c6> ));
    MPL_ASSERT(( has_yyy<c7> ));

    // Different name, different args.
    MPL_ASSERT_NOT(( has_xxx_template<c6> ));
    MPL_ASSERT_NOT(( has_xxx_template<c7> ));
    MPL_ASSERT_NOT(( has_yyy<c1> ));
    MPL_ASSERT_NOT(( has_yyy<c2> ));
    MPL_ASSERT_NOT(( has_yyy<c3> ));
    MPL_ASSERT_NOT(( has_yyy<c4> ));
    MPL_ASSERT_NOT(( has_yyy<c5> ));

    MPL_ASSERT(( has_xxx<b1,true_> ));
    MPL_ASSERT(( has_xxx<b2,true_> ));
    MPL_ASSERT(( has_xxx<b3,true_> ));
    MPL_ASSERT(( has_xxx<b4,true_> ));
    MPL_ASSERT(( has_xxx<b5,true_> ));
    MPL_ASSERT(( has_xxx<b6,true_> ));
    MPL_ASSERT(( has_xxx<b7,true_> ));

    MPL_ASSERT(( has_xxx_template<c1,true_> ));

#if !defined(HAS_XXX_ASSERT)
#   define HAS_XXX_ASSERT(x) MPL_ASSERT(x)
#endif

    HAS_XXX_ASSERT(( has_xxx<b1> ));
    HAS_XXX_ASSERT(( has_xxx<b2> ));
    HAS_XXX_ASSERT(( has_xxx<b3> ));
    HAS_XXX_ASSERT(( has_xxx<b4> ));
    HAS_XXX_ASSERT(( has_xxx<b5> ));
    HAS_XXX_ASSERT(( has_xxx<b6> ));
    HAS_XXX_ASSERT(( has_xxx<b7> ));

#if !defined(HAS_XXX_TEMPLATE_ASSERT)
#   define HAS_XXX_TEMPLATE_ASSERT(x) MPL_ASSERT(x)
#endif

    HAS_XXX_TEMPLATE_ASSERT(( has_xxx_template<c1> ));
}
