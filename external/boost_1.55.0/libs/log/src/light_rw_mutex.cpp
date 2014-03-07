/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   light_rw_mutex.cpp
 * \author Andrey Semashev
 * \date   19.06.2010
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

// This first include is to ensure that __MSVCRT_VERSION__ is defined properly
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/light_rw_mutex.hpp>

#if !defined(BOOST_LOG_NO_THREADS)

#if !defined(BOOST_LOG_LWRWMUTEX_USE_PTHREAD) && !defined(BOOST_LOG_LWRWMUTEX_USE_SRWLOCK)

#include <new>
#include <boost/assert.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/log/utility/once_block.hpp>
#include <boost/log/detail/malloc_aligned.hpp>

#include "windows_version.hpp"
#include <windows.h>

#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

BOOST_LOG_ANONYMOUS_NAMESPACE {

struct mutex_impl { void* p; }; // has the same layout as SRWLOCK and light_rw_mutex::m_Mutex

typedef void (__stdcall *init_fun_t)(mutex_impl*);
typedef void (__stdcall *destroy_fun_t)(mutex_impl*);
typedef void (__stdcall *lock_exclusive_fun_t)(mutex_impl*);
typedef void (__stdcall *lock_shared_fun_t)(mutex_impl*);
typedef void (__stdcall *unlock_exclusive_fun_t)(mutex_impl*);
typedef void (__stdcall *unlock_shared_fun_t)(mutex_impl*);

//! A complement stub function for InitializeSRWLock
void __stdcall DeinitializeSRWLock(mutex_impl*)
{
}

// The Boost.Thread-based implementation
void __stdcall InitializeSharedMutex(mutex_impl* mtx)
{
    // To avoid cache line aliasing we do aligned memory allocation here
    enum
    {
        // Allocation size is the minimum number of cache lines to accommodate shared_mutex
        size =
            (
                sizeof(shared_mutex) / BOOST_LOG_CPU_CACHE_LINE_SIZE
                + ((sizeof(shared_mutex) % BOOST_LOG_CPU_CACHE_LINE_SIZE) != 0)
            )
            * BOOST_LOG_CPU_CACHE_LINE_SIZE
    };
    mtx->p = malloc_aligned(size, BOOST_LOG_CPU_CACHE_LINE_SIZE);
    BOOST_ASSERT(mtx->p != NULL);
    new (mtx->p) shared_mutex();
}

void __stdcall DeinitializeSharedMutex(mutex_impl* mtx)
{
    static_cast< shared_mutex* >(mtx->p)->~shared_mutex();
    free_aligned(mtx->p);
    mtx->p = NULL;
}

void __stdcall ExclusiveLockSharedMutex(mutex_impl* mtx)
{
    static_cast< shared_mutex* >(mtx->p)->lock();
}

void __stdcall SharedLockSharedMutex(mutex_impl* mtx)
{
    static_cast< shared_mutex* >(mtx->p)->lock_shared();
}

void __stdcall ExclusiveUnlockSharedMutex(mutex_impl* mtx)
{
    static_cast< shared_mutex* >(mtx->p)->unlock();
}

void __stdcall SharedUnlockSharedMutex(mutex_impl* mtx)
{
    static_cast< shared_mutex* >(mtx->p)->unlock_shared();
}

// Pointers to the actual implementation functions
init_fun_t g_pInitializeLWRWMutex = NULL;
destroy_fun_t g_pDestroyLWRWMutex = NULL;
lock_exclusive_fun_t g_pLockExclusiveLWRWMutex = NULL;
lock_shared_fun_t g_pLockSharedLWRWMutex = NULL;
unlock_exclusive_fun_t g_pUnlockExclusiveLWRWMutex = NULL;
unlock_shared_fun_t g_pUnlockSharedLWRWMutex = NULL;

//! The function dynamically initializes the implementation pointers
void init_light_rw_mutex_impl()
{
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (hKernel32)
    {
        g_pInitializeLWRWMutex =
            (init_fun_t)GetProcAddress(hKernel32, "InitializeSRWLock");
        if (g_pInitializeLWRWMutex)
        {
            g_pLockExclusiveLWRWMutex =
                (lock_exclusive_fun_t)GetProcAddress(hKernel32, "AcquireSRWLockExclusive");
            if (g_pLockExclusiveLWRWMutex)
            {
                g_pUnlockExclusiveLWRWMutex =
                    (unlock_exclusive_fun_t)GetProcAddress(hKernel32, "ReleaseSRWLockExclusive");
                if (g_pUnlockExclusiveLWRWMutex)
                {
                    g_pLockSharedLWRWMutex =
                        (lock_shared_fun_t)GetProcAddress(hKernel32, "AcquireSRWLockShared");
                    if (g_pLockSharedLWRWMutex)
                    {
                        g_pUnlockSharedLWRWMutex =
                            (unlock_shared_fun_t)GetProcAddress(hKernel32, "ReleaseSRWLockShared");
                        if (g_pUnlockSharedLWRWMutex)
                        {
                            g_pDestroyLWRWMutex = &DeinitializeSRWLock;
                            return;
                        }
                    }
                }
            }
        }
    }

    // Current OS doesn't have support for SRWLOCK, use Boost.Thread instead
    g_pInitializeLWRWMutex = &InitializeSharedMutex;
    g_pDestroyLWRWMutex = &DeinitializeSharedMutex;
    g_pLockExclusiveLWRWMutex = &ExclusiveLockSharedMutex;
    g_pUnlockExclusiveLWRWMutex = &ExclusiveUnlockSharedMutex;
    g_pLockSharedLWRWMutex = &SharedLockSharedMutex;
    g_pUnlockSharedLWRWMutex = &SharedUnlockSharedMutex;
}

} // namespace

BOOST_LOG_API light_rw_mutex::light_rw_mutex()
{
    BOOST_LOG_ONCE_BLOCK()
    {
        init_light_rw_mutex_impl();
    }
    g_pInitializeLWRWMutex((mutex_impl*)&m_Mutex);
}

BOOST_LOG_API light_rw_mutex::~light_rw_mutex()
{
    g_pDestroyLWRWMutex((mutex_impl*)&m_Mutex);
}

BOOST_LOG_API void light_rw_mutex::lock_shared()
{
    g_pLockSharedLWRWMutex((mutex_impl*)&m_Mutex);
}

BOOST_LOG_API void light_rw_mutex::unlock_shared()
{
    g_pUnlockSharedLWRWMutex((mutex_impl*)&m_Mutex);
}

BOOST_LOG_API void light_rw_mutex::lock()
{
    g_pLockExclusiveLWRWMutex((mutex_impl*)&m_Mutex);
}

BOOST_LOG_API void light_rw_mutex::unlock()
{
    g_pUnlockExclusiveLWRWMutex((mutex_impl*)&m_Mutex);
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // !defined(BOOST_LOG_LWRWMUTEX_USE_PTHREAD) && !defined(BOOST_LOG_LWRWMUTEX_USE_SRWLOCK)

#endif // !defined(BOOST_LOG_NO_THREADS)
