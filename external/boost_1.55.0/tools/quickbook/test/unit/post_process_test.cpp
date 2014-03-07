/*=============================================================================
    Copyright (c) 2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "post_process.hpp"
#include <boost/detail/lightweight_test.hpp>

#define EXPECT_EXCEPTION(test, msg) \
    try { \
        test; \
        BOOST_ERROR(msg); \
    } \
    catch(quickbook::post_process_failure&) { \
    }

int main()
{
    EXPECT_EXCEPTION(
        quickbook::post_process("</thing>"),
        "Succeeded with unbalanced tag");
    EXPECT_EXCEPTION(
        quickbook::post_process("<"),
        "Succeeded with badly formed tag");

    return boost::report_errors();
}
