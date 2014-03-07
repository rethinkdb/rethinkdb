/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   windows_version.hpp
 * \author Andrey Semashev
 * \date   07.03.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_WINDOWS_VERSION_HPP_INCLUDED_
#define BOOST_LOG_WINDOWS_VERSION_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#if defined(BOOST_WINDOWS) || defined(__CYGWIN__)

#if defined(BOOST_LOG_USE_WINNT6_API)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // _WIN32_WINNT_LONGHORN
#endif

#else

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500 // _WIN32_WINNT_WIN2K
#endif

#endif // BOOST_LOG_USE_WINNT6_API

// This is to make Boost.ASIO happy
#ifndef __USE_W32_SOCKETS
#define __USE_W32_SOCKETS
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#endif // defined(BOOST_WINDOWS) || defined(__CYGWIN__)

#endif // BOOST_LOG_WINDOWS_VERSION_HPP_INCLUDED_
