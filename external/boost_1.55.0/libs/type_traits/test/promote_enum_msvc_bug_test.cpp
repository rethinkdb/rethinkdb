// Copyright 2009 Alexander Nasonov.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Test bug 99776 'enum UIntEnum { value = UINT_MAX } is promoted to int'
// http://lab.msdn.microsoft.com/ProductFeedback/viewfeedback.aspx?feedbackid=22b0a6b7-120f-4ca0-9136-fa1b25b26efe
//
// Intel 9.0.028 for Windows has a similar problem:
// https://premier.intel.com/IssueDetail.aspx?IssueID=365073

#include <boost/type_traits/promote.hpp>

#include <climits>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include "promote_util.hpp"

#ifdef BOOST_INTEL
//  remark #1418: external function definition with no prior declaration
#pragma warning(disable:1418)
#endif


enum UIntEnum { UIntEnum_max = UINT_MAX };

template<class T>
void assert_is_uint(T)
{
    BOOST_STATIC_ASSERT((boost::is_same<T, unsigned int>::value));
}

void test_promote_to_uint()
{
    assert_is_uint(+UIntEnum_max);
    test_cv< UIntEnum, unsigned int >();
}

int main()
{
    return 0;
}

