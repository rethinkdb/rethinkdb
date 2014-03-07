/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   unhandled_exception_count.cpp
 * \author Andrey Semashev
 * \date   05.11.2012
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 *
 * The code in this file is based on the implementation by Evgeny Panasyuk:
 *
 * https://github.com/panaseleus/stack_unwinding/blob/master/boost/exception/uncaught_exception_count.hpp
 */

#include <exception>
#include <boost/log/detail/unhandled_exception_count.hpp>
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

BOOST_LOG_ANONYMOUS_NAMESPACE {

#if defined(BOOST_LOG_HAS_CXXABI_H)
// MinGW GCC 4.4 seem to not work the same way the newer GCC versions do. As a result, __cxa_get_globals based implementation will always return 0.
// Just disable it for now and fall back to std::uncaught_exception().
#if !defined(__MINGW32__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)))
// Only GCC 4.7 declares __cxa_get_globals() in cxxabi.h, older compilers do not expose this function but it's there
#define BOOST_LOG_HAS_CXA_GET_GLOBALS
extern "C" void* __cxa_get_globals();
#endif
#elif defined(_MSC_VER) && _MSC_VER >= 1400
#define BOOST_LOG_HAS_GETPTD
extern "C" void* _getptd();
#endif

} // namespace

//! Returns the number of currently pending exceptions
BOOST_LOG_API unsigned int unhandled_exception_count() BOOST_NOEXCEPT
{
#if defined(BOOST_LOG_HAS_CXA_GET_GLOBALS)
    // Tested on {clang 3.2,GCC 3.5.6,GCC 4.1.2,GCC 4.4.6,GCC 4.4.7}x{x32,x64}
    return *(reinterpret_cast< const unsigned int* >(static_cast< const char* >(__cxa_get_globals()) + sizeof(void*))); // __cxa_eh_globals::uncaughtExceptions, x32 offset - 0x4, x64 - 0x8
#elif defined(BOOST_LOG_HAS_GETPTD)
    // MSVC specific. Tested on {MSVC2005SP1,MSVC2008SP1,MSVC2010SP1,MSVC2012}x{x32,x64}.
    return *(reinterpret_cast< const unsigned int* >(static_cast< const char* >(_getptd()) + (sizeof(void*) == 8 ? 0x100 : 0x90))); // _tiddata::_ProcessingThrow, x32 offset - 0x90, x64 - 0x100
#else
    // Portable implementation. Does not allow to detect multiple nested exceptions.
    return static_cast< unsigned int >(std::uncaught_exception());
#endif
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>
