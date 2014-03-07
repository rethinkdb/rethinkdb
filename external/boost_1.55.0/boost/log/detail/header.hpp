/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/config/abi_prefix.hpp>

#if !defined(BOOST_LOG_ENABLE_WARNINGS)

#if defined(_MSC_VER)

#pragma warning(push, 3)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
// switch statement contains 'default' but no 'case' labels
#pragma warning(disable: 4065)
// 'this' : used in base member initializer list
#pragma warning(disable: 4355)
// 'int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4800)
// unreferenced formal parameter
#pragma warning(disable: 4100)
// conditional expression is constant
#pragma warning(disable: 4127)
// default constructor could not be generated
#pragma warning(disable: 4510)
// copy constructor could not be generated
#pragma warning(disable: 4511)
// assignment operator could not be generated
#pragma warning(disable: 4512)
// struct 'A' can never be instantiated - user defined constructor required
#pragma warning(disable: 4610)
// function marked as __forceinline not inlined
#pragma warning(disable: 4714)
// decorated name length exceeded, name was truncated
#pragma warning(disable: 4503)

#elif defined(__GNUC__) && !(defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)) \
    && (__GNUC__ * 100 + __GNUC_MINOR__) >= 406

#pragma GCC diagnostic push
// 'var' defined but not used
#pragma GCC diagnostic ignored "-Wunused-variable"
// unused parameter 'arg'
#pragma GCC diagnostic ignored "-Wunused-parameter"
// missing initializer for member var
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#if (__GNUC__ * 100 + __GNUC_MINOR__) >= 407
// typedef ‘foo’ locally defined but not used
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

#endif

#endif // !defined(BOOST_LOG_ENABLE_WARNINGS)
