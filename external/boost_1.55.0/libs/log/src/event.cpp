/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   event.cpp
 * \author Andrey Semashev
 * \date   24.07.2011
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#include <boost/log/detail/config.hpp>

#ifndef BOOST_LOG_NO_THREADS

#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/throw_exception.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/log/detail/event.hpp>

#if defined(BOOST_LOG_EVENT_USE_POSIX_SEMAPHORE)

#if defined(__GNUC__) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
#define BOOST_LOG_EVENT_TRY_SET(ref) (__sync_lock_test_and_set(&ref, 1U) == 0U)
#define BOOST_LOG_EVENT_RESET(ref) __sync_lock_release(&ref)
#else
#error Boost.Log internal error: BOOST_LOG_EVENT_USE_POSIX_SEMAPHORE must only be defined when atomic ops are available
#endif
#include <errno.h>
#include <semaphore.h>

#elif defined(BOOST_LOG_EVENT_USE_WINAPI)

#include "windows_version.hpp"
#include <windows.h>
#include <boost/detail/interlocked.hpp>

#else

#include <boost/thread/locks.hpp>

#endif

#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

#if defined(BOOST_LOG_EVENT_USE_POSIX_SEMAPHORE)

//! Default constructor
BOOST_LOG_API sem_based_event::sem_based_event() : m_state(0U)
{
    if (sem_init(&m_semaphore, 0, 0) != 0)
    {
        const int err = errno;
        BOOST_THROW_EXCEPTION(system::system_error(
            err, system::system_category(), "Failed to initialize semaphore"));
    }
}

//! Destructor
BOOST_LOG_API sem_based_event::~sem_based_event()
{
    BOOST_VERIFY(sem_destroy(&m_semaphore) == 0);
}

//! Waits for the object to become signalled
BOOST_LOG_API void sem_based_event::wait()
{
    while (true)
    {
        if (sem_wait(&m_semaphore) != 0)
        {
            const int err = errno;
            if (err != EINTR)
            {
                BOOST_THROW_EXCEPTION(system::system_error(
                    err, system::system_category(), "Failed to block on the semaphore"));
            }
        }
        else
            break;
    }
    BOOST_LOG_EVENT_RESET(m_state);
}

//! Sets the object to a signalled state
BOOST_LOG_API void sem_based_event::set_signalled()
{
    if (BOOST_LOG_EVENT_TRY_SET(m_state))
    {
        if (sem_post(&m_semaphore) != 0)
        {
            const int err = errno;
            BOOST_LOG_EVENT_RESET(m_state);
            BOOST_THROW_EXCEPTION(system::system_error(
                err, system::system_category(), "Failed to wake the blocked thread"));
        }
    }
}

#elif defined(BOOST_LOG_EVENT_USE_WINAPI)

//! Default constructor
BOOST_LOG_API winapi_based_event::winapi_based_event() :
    m_state(0),
    m_event(CreateEventA(NULL, false, false, NULL))
{
    if (!m_event)
    {
        const DWORD err = GetLastError();
        BOOST_THROW_EXCEPTION(system::system_error(
            err, system::system_category(), "Failed to create Windows event"));
    }
}

//! Destructor
BOOST_LOG_API winapi_based_event::~winapi_based_event()
{
    BOOST_VERIFY(CloseHandle(m_event) != 0);
}

//! Waits for the object to become signalled
BOOST_LOG_API void winapi_based_event::wait()
{
    // On Windows we assume that memory view is always actual (Intel x86 and x86_64 arch)
    if (const_cast< volatile boost::uint32_t& >(m_state) == 0)
    {
        if (WaitForSingleObject(m_event, INFINITE) != 0)
        {
            const DWORD err = GetLastError();
            BOOST_THROW_EXCEPTION(system::system_error(
                err, system::system_category(), "Failed to block on Windows event"));
        }
    }
    const_cast< volatile boost::uint32_t& >(m_state) = 0;
}

//! Sets the object to a signalled state
BOOST_LOG_API void winapi_based_event::set_signalled()
{
    if (BOOST_INTERLOCKED_COMPARE_EXCHANGE(reinterpret_cast< long* >(&m_state), 1, 0) == 0)
    {
        if (SetEvent(m_event) == 0)
        {
            const DWORD err = GetLastError();
            const_cast< volatile boost::uint32_t& >(m_state) = 0;
            BOOST_THROW_EXCEPTION(system::system_error(
                err, system::system_category(), "Failed to wake the blocked thread"));
        }
    }
}

#else

//! Default constructor
BOOST_LOG_API generic_event::generic_event() : m_state(false)
{
}

//! Destructor
BOOST_LOG_API generic_event::~generic_event()
{
}

//! Waits for the object to become signalled
BOOST_LOG_API void generic_event::wait()
{
    boost::unique_lock< boost::mutex > lock(m_mutex);
    while (!m_state)
    {
        m_cond.wait(lock);
    }
    m_state = false;
}

//! Sets the object to a signalled state
BOOST_LOG_API void generic_event::set_signalled()
{
    boost::lock_guard< boost::mutex > lock(m_mutex);
    if (!m_state)
    {
        m_state = true;
        m_cond.notify_one();
    }
}

#endif

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_NO_THREADS
