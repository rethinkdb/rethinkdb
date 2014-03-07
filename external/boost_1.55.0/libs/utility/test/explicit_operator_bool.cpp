/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   explicit_operator_bool_compile.cpp
 * \author Andrey Semashev
 * \date   17.07.2010
 *
 * \brief  This test checks that explicit operator bool can be used in
 *         the valid contexts.
 */

#define BOOST_TEST_MODULE explicit_operator_bool_compile

#include <boost/utility/explicit_operator_bool.hpp>

namespace {

    // A test object that has the operator of explicit conversion to bool
    struct checkable1
    {
        BOOST_EXPLICIT_OPERATOR_BOOL()
        bool operator! () const
        {
            return false;
        }
    };

    struct checkable2
    {
        BOOST_CONSTEXPR_EXPLICIT_OPERATOR_BOOL()
        BOOST_CONSTEXPR bool operator! () const
        {
            return false;
        }
    };

} // namespace

int main(int, char*[])
{
    checkable1 val1;
    if (val1)
    {
        checkable2 val2;
        if (val2)
            return 0;
    }

    return 1;
}
