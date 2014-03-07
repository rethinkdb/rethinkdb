/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   process_id.cpp
 * \author Andrey Semashev
 * \date   12.09.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#include <iostream>
#include <boost/integer.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/log/detail/process_id.hpp>
#include <boost/log/detail/header.hpp>

#if defined(BOOST_WINDOWS)

#define WIN32_LEAN_AND_MEAN

#include "windows_version.hpp"
#include <windows.h>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

enum { pid_size = sizeof(GetCurrentProcessId()) };

namespace this_process {

    //! The function returns current process identifier
    BOOST_LOG_API process::id get_id()
    {
        return process::id(GetCurrentProcessId());
    }

} // namespace this_process

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#else // defined(BOOST_WINDOWS)

#include <unistd.h>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

namespace this_process {

    //! The function returns current process identifier
    BOOST_LOG_API process::id get_id()
    {
        // According to POSIX, pid_t should always be an integer type:
        // http://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/types.h.html
        return process::id(getpid());
    }

} // namespace this_process

enum { pid_size = sizeof(pid_t) };

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // defined(BOOST_WINDOWS)


namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

template< typename CharT, typename TraitsT >
std::basic_ostream< CharT, TraitsT >&
operator<< (std::basic_ostream< CharT, TraitsT >& strm, process::id const& pid)
{
    if (strm.good())
    {
        io::ios_flags_saver flags_saver(strm, std::ios_base::hex | std::ios_base::showbase);
        // The width is set calculated to accommodate pid in hex + "0x" prefix
        io::ios_width_saver width_saver(strm, static_cast< std::streamsize >(pid_size * 2 + 2));
        io::basic_ios_fill_saver< CharT, TraitsT > fill_saver(strm, static_cast< CharT >('0'));
        strm << static_cast< uint_t< pid_size * 8 >::least >(pid.native_id());
    }

    return strm;
}

#if defined(BOOST_LOG_USE_CHAR)
template BOOST_LOG_API
std::basic_ostream< char, std::char_traits< char > >&
operator<< (std::basic_ostream< char, std::char_traits< char > >& strm, process::id const& pid);
#endif // defined(BOOST_LOG_USE_CHAR)

#if defined(BOOST_LOG_USE_WCHAR_T)
template BOOST_LOG_API
std::basic_ostream< wchar_t, std::char_traits< wchar_t > >&
operator<< (std::basic_ostream< wchar_t, std::char_traits< wchar_t > >& strm, process::id const& pid);
#endif // defined(BOOST_LOG_USE_WCHAR_T)

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>
