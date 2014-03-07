
// Copyright Steven Watanabe 2009
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: push_back.cpp 55647 2009-08-18 05:00:17Z agurtovoy $
// $Date: 2008-10-10 02:21:07 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49240 $

#include <boost/mpl/push_back.hpp>

#include <boost/mpl/aux_/test.hpp>

struct no_push_back_tag {};

struct no_push_back
{
    typedef no_push_back_tag tag;
};

struct has_push_back_tag {};

struct with_push_back
{
    typedef has_push_back_tag tag;
};

namespace boost { namespace mpl {

template<>
struct push_back_impl< has_push_back_tag >
{
    template<class Seq, class T> struct apply
    {
        typedef no_push_back type;
    };
};

}}

MPL_TEST_CASE()
{
    MPL_ASSERT_NOT(( has_push_back< no_push_back > ));
    MPL_ASSERT(( has_push_back< with_push_back > ));

    typedef push_back< with_push_back , int >::type test;
    MPL_ASSERT(( is_same< test, no_push_back > ));
}
