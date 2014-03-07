/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   light_rw_mutex.hpp
 * \author Andrey Semashev
 * \date   24.03.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_LIGHT_RW_MUTEX_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_LIGHT_RW_MUTEX_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#ifndef BOOST_LOG_NO_THREADS

#include <boost/log/detail/header.hpp>

#if defined(BOOST_THREAD_POSIX) // This one can be defined by users, so it should go first
#define BOOST_LOG_LWRWMUTEX_USE_PTHREAD
#elif defined(BOOST_WINDOWS) && defined(BOOST_LOG_USE_WINNT6_API)
#define BOOST_LOG_LWRWMUTEX_USE_SRWLOCK
#elif defined(BOOST_HAS_PTHREADS)
#define BOOST_LOG_LWRWMUTEX_USE_PTHREAD
#endif

#if defined(BOOST_LOG_LWRWMUTEX_USE_SRWLOCK)

#if defined(BOOST_USE_WINDOWS_H)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // _WIN32_WINNT_LONGHORN
#endif

#include <windows.h>

#else // defined(BOOST_USE_WINDOWS_H)

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

extern "C" {

struct SRWLOCK { void* p; };
__declspec(dllimport) void __stdcall InitializeSRWLock(SRWLOCK*);
__declspec(dllimport) void __stdcall ReleaseSRWLockExclusive(SRWLOCK*);
__declspec(dllimport) void __stdcall ReleaseSRWLockShared(SRWLOCK*);
__declspec(dllimport) void __stdcall AcquireSRWLockExclusive(SRWLOCK*);
__declspec(dllimport) void __stdcall AcquireSRWLockShared(SRWLOCK*);

} // extern "C"

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_USE_WINDOWS_H

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! A light read/write mutex that uses WinNT 6 and later APIs
class light_rw_mutex
{
    SRWLOCK m_Mutex;

public:
    light_rw_mutex()
    {
        InitializeSRWLock(&m_Mutex);
    }
    void lock_shared()
    {
        AcquireSRWLockShared(&m_Mutex);
    }
    void unlock_shared()
    {
        ReleaseSRWLockShared(&m_Mutex);
    }
    void lock()
    {
        AcquireSRWLockExclusive(&m_Mutex);
    }
    void unlock()
    {
        ReleaseSRWLockExclusive(&m_Mutex);
    }

    // Noncopyable
    BOOST_DELETED_FUNCTION(light_rw_mutex(light_rw_mutex const&))
    BOOST_DELETED_FUNCTION(light_rw_mutex& operator= (light_rw_mutex const&))
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#elif defined(BOOST_LOG_LWRWMUTEX_USE_PTHREAD)

#include <pthread.h>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! A light read/write mutex that maps directly onto POSIX threading library
class light_rw_mutex
{
    pthread_rwlock_t m_Mutex;

public:
    light_rw_mutex()
    {
        pthread_rwlock_init(&m_Mutex, NULL);
    }
    ~light_rw_mutex()
    {
        pthread_rwlock_destroy(&m_Mutex);
    }
    void lock_shared()
    {
        pthread_rwlock_rdlock(&m_Mutex);
    }
    void unlock_shared()
    {
        pthread_rwlock_unlock(&m_Mutex);
    }
    void lock()
    {
        pthread_rwlock_wrlock(&m_Mutex);
    }
    void unlock()
    {
        pthread_rwlock_unlock(&m_Mutex);
    }

    // Noncopyable
    BOOST_DELETED_FUNCTION(light_rw_mutex(light_rw_mutex const&))
    BOOST_DELETED_FUNCTION(light_rw_mutex& operator= (light_rw_mutex const&))
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#else

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! A light read/write mutex
class light_rw_mutex
{
    struct { void* p; } m_Mutex;

public:
    BOOST_LOG_API light_rw_mutex();
    BOOST_LOG_API ~light_rw_mutex();
    BOOST_LOG_API void lock_shared();
    BOOST_LOG_API void unlock_shared();
    BOOST_LOG_API void lock();
    BOOST_LOG_API void unlock();

    // Noncopyable
    BOOST_DELETED_FUNCTION(light_rw_mutex(light_rw_mutex const&))
    BOOST_DELETED_FUNCTION(light_rw_mutex& operator= (light_rw_mutex const&))
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_NO_THREADS

#endif // BOOST_LOG_DETAIL_LIGHT_RW_MUTEX_HPP_INCLUDED_
