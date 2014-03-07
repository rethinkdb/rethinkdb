/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   current_function_support.cpp
 * \author Andrey Semashev
 * \date   26.09.2010
 *
 * \brief  This test checks that the BOOST_CURRENT_FUNCTION macro has semantics
 *         compatible with Boost.Log on the current platform.
 *
 * The point of this test is to determine whether the macro unfolds into a string literal
 * rather than a pointer to a string. This is critical because BOOST_LOG_WFUNCTION
 * relies on this fact - it determines the length of the literal by applying sizeof to it.
 */

#define BOOST_TEST_MODULE current_function_support

#include <boost/current_function.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_array.hpp>

template< typename T >
void check(T& param)
{
    BOOST_STATIC_ASSERT(boost::is_array< T >::value);
}

int main(int, char*[])
{
    check(BOOST_CURRENT_FUNCTION);

    return 0;
}
