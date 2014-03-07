/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   explicit_operator_bool_compile_fail_shift.cpp
 * \author Andrey Semashev
 * \date   17.07.2010
 *
 * \brief  This test checks that explicit operator bool cannot be used in
 *         an unintended context.
 */

#define BOOST_TEST_MODULE explicit_operator_bool_compile_fail_shift

#include <boost/utility/explicit_operator_bool.hpp>

namespace {

    // A test object that has the operator of explicit conversion to bool
    struct checkable
    {
        BOOST_EXPLICIT_OPERATOR_BOOL()
        bool operator! () const
        {
            return false;
        }
    };

} // namespace

int main(int, char*[])
{
    checkable val;
    val << 2;

    return 0;
}
