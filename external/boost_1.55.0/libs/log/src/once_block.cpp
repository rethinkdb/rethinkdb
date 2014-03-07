/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   once_block.cpp
 * \author Andrey Semashev
 * \date   23.06.2010
 *
 * \brief  This file is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 *
 * The code in this file is based on the \c call_once function implementation in Boost.Thread.
 */

#include <boost/log/utility/once_block.hpp>
#ifndef BOOST_LOG_NO_THREADS

#include <cstdlib>
#include <boost/assert.hpp>

#if defined(BOOST_THREAD_PLATFORM_WIN32)

#include "windows_version.hpp"
#include <windows.h>

#if defined(BOOST_LOG_USE_WINNT6_API)

#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

BOOST_LOG_ANONYMOUS_NAMESPACE {

    SRWLOCK g_OnceBlockMutex = SRWLOCK_INIT;
    CONDITION_VARIABLE g_OnceBlockCond = CONDITION_VARIABLE_INIT;

} // namespace

BOOST_LOG_API bool once_block_sentry::enter_once_block() const BOOST_NOEXCEPT
{
    AcquireSRWLockExclusive(&g_OnceBlockMutex);

    once_block_flag volatile& flag = m_flag;
    while (flag.status != once_block_flag::initialized)
    {
        if (flag.status == once_block_flag::uninitialized)
        {
            flag.status = once_block_flag::being_initialized;
            ReleaseSRWLockExclusive(&g_OnceBlockMutex);

            // Invoke the initializer block
            return false;
        }
        else
        {
            while (flag.status == once_block_flag::being_initialized)
            {
                BOOST_VERIFY(SleepConditionVariableSRW(
                    &g_OnceBlockCond, &g_OnceBlockMutex, INFINITE, 0));
            }
        }
    }

    ReleaseSRWLockExclusive(&g_OnceBlockMutex);

    return true;
}

BOOST_LOG_API void once_block_sentry::commit() BOOST_NOEXCEPT
{
    AcquireSRWLockExclusive(&g_OnceBlockMutex);

    // The initializer executed successfully
    m_flag.status = once_block_flag::initialized;

    ReleaseSRWLockExclusive(&g_OnceBlockMutex);
    WakeAllConditionVariable(&g_OnceBlockCond);
}

BOOST_LOG_API void once_block_sentry::rollback() BOOST_NOEXCEPT
{
    AcquireSRWLockExclusive(&g_OnceBlockMutex);

    // The initializer failed, marking the flag as if it hasn't run at all
    m_flag.status = once_block_flag::uninitialized;

    ReleaseSRWLockExclusive(&g_OnceBlockMutex);
    WakeAllConditionVariable(&g_OnceBlockCond);
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#else // defined(BOOST_LOG_USE_WINNT6_API)

#include <cstdlib> // atexit
#include <boost/detail/interlocked.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

BOOST_LOG_ANONYMOUS_NAMESPACE {

    struct BOOST_LOG_NO_VTABLE once_block_impl_base
    {
        virtual ~once_block_impl_base() {}
        virtual bool enter_once_block(once_block_flag volatile& flag) = 0;
        virtual void commit(once_block_flag& flag) = 0;
        virtual void rollback(once_block_flag& flag) = 0;
    };

    class once_block_impl_nt6 :
        public once_block_impl_base
    {
    public:
        struct SRWLOCK { void* p; };
        struct CONDITION_VARIABLE { void* p; };

        typedef void (__stdcall *InitializeSRWLock_t)(SRWLOCK*);
        typedef void (__stdcall *AcquireSRWLockExclusive_t)(SRWLOCK*);
        typedef void (__stdcall *ReleaseSRWLockExclusive_t)(SRWLOCK*);
        typedef void (__stdcall *InitializeConditionVariable_t)(CONDITION_VARIABLE*);
        typedef BOOL (__stdcall *SleepConditionVariableSRW_t)(CONDITION_VARIABLE*, SRWLOCK*, DWORD, ULONG);
        typedef void (__stdcall *WakeAllConditionVariable_t)(CONDITION_VARIABLE*);

    private:
        SRWLOCK m_Mutex;
        CONDITION_VARIABLE m_Cond;

        AcquireSRWLockExclusive_t m_pAcquireSRWLockExclusive;
        ReleaseSRWLockExclusive_t m_pReleaseSRWLockExclusive;
        SleepConditionVariableSRW_t m_pSleepConditionVariableSRW;
        WakeAllConditionVariable_t m_pWakeAllConditionVariable;

    public:
        once_block_impl_nt6(
            InitializeSRWLock_t pInitializeSRWLock,
            AcquireSRWLockExclusive_t pAcquireSRWLockExclusive,
            ReleaseSRWLockExclusive_t pReleaseSRWLockExclusive,
            InitializeConditionVariable_t pInitializeConditionVariable,
            SleepConditionVariableSRW_t pSleepConditionVariableSRW,
            WakeAllConditionVariable_t pWakeAllConditionVariable
        ) :
            m_pAcquireSRWLockExclusive(pAcquireSRWLockExclusive),
            m_pReleaseSRWLockExclusive(pReleaseSRWLockExclusive),
            m_pSleepConditionVariableSRW(pSleepConditionVariableSRW),
            m_pWakeAllConditionVariable(pWakeAllConditionVariable)
        {
            pInitializeSRWLock(&m_Mutex);
            pInitializeConditionVariable(&m_Cond);
        }

        bool enter_once_block(once_block_flag volatile& flag)
        {
            m_pAcquireSRWLockExclusive(&m_Mutex);

            while (flag.status != once_block_flag::initialized)
            {
                if (flag.status == once_block_flag::uninitialized)
                {
                    flag.status = once_block_flag::being_initialized;
                    m_pReleaseSRWLockExclusive(&m_Mutex);

                    // Invoke the initializer block
                    return false;
                }
                else
                {
                    while (flag.status == once_block_flag::being_initialized)
                    {
                        BOOST_VERIFY(m_pSleepConditionVariableSRW(
                            &m_Cond, &m_Mutex, INFINITE, 0));
                    }
                }
            }

            m_pReleaseSRWLockExclusive(&m_Mutex);

            return true;
        }

        void commit(once_block_flag& flag)
        {
            m_pAcquireSRWLockExclusive(&m_Mutex);

            // The initializer executed successfully
            flag.status = once_block_flag::initialized;

            m_pReleaseSRWLockExclusive(&m_Mutex);
            m_pWakeAllConditionVariable(&m_Cond);
        }

        void rollback(once_block_flag& flag)
        {
            m_pAcquireSRWLockExclusive(&m_Mutex);

            // The initializer failed, marking the flag as if it hasn't run at all
            flag.status = once_block_flag::uninitialized;

            m_pReleaseSRWLockExclusive(&m_Mutex);
            m_pWakeAllConditionVariable(&m_Cond);
        }
    };

    class once_block_impl_nt5 :
        public once_block_impl_base
    {
    private:
        mutex m_Mutex;
        condition_variable m_Cond;

    public:
        bool enter_once_block(once_block_flag volatile& flag)
        {
            unique_lock< mutex > lock(m_Mutex);

            while (flag.status != once_block_flag::initialized)
            {
                if (flag.status == once_block_flag::uninitialized)
                {
                    flag.status = once_block_flag::being_initialized;

                    // Invoke the initializer block
                    return false;
                }
                else
                {
                    while (flag.status == once_block_flag::being_initialized)
                    {
                        m_Cond.wait(lock);
                    }
                }
            }

            return true;
        }

        void commit(once_block_flag& flag)
        {
            {
                lock_guard< mutex > _(m_Mutex);
                flag.status = once_block_flag::initialized;
            }
            m_Cond.notify_all();
        }

        void rollback(once_block_flag& flag)
        {
            {
                lock_guard< mutex > _(m_Mutex);
                flag.status = once_block_flag::uninitialized;
            }
            m_Cond.notify_all();
        }
    };

    once_block_impl_base* create_once_block_impl()
    {
        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
        if (hKernel32)
        {
            once_block_impl_nt6::InitializeSRWLock_t pInitializeSRWLock =
                (once_block_impl_nt6::InitializeSRWLock_t)GetProcAddress(hKernel32, "InitializeSRWLock");
            if (pInitializeSRWLock)
            {
                once_block_impl_nt6::AcquireSRWLockExclusive_t pAcquireSRWLockExclusive =
                    (once_block_impl_nt6::AcquireSRWLockExclusive_t)GetProcAddress(hKernel32, "AcquireSRWLockExclusive");
                if (pAcquireSRWLockExclusive)
                {
                    once_block_impl_nt6::ReleaseSRWLockExclusive_t pReleaseSRWLockExclusive =
                        (once_block_impl_nt6::ReleaseSRWLockExclusive_t)GetProcAddress(hKernel32, "ReleaseSRWLockExclusive");
                    if (pReleaseSRWLockExclusive)
                    {
                        once_block_impl_nt6::InitializeConditionVariable_t pInitializeConditionVariable =
                            (once_block_impl_nt6::InitializeConditionVariable_t)GetProcAddress(hKernel32, "InitializeConditionVariable");
                        if (pInitializeConditionVariable)
                        {
                            once_block_impl_nt6::SleepConditionVariableSRW_t pSleepConditionVariableSRW =
                                (once_block_impl_nt6::SleepConditionVariableSRW_t)GetProcAddress(hKernel32, "SleepConditionVariableSRW");
                            if (pSleepConditionVariableSRW)
                            {
                                once_block_impl_nt6::WakeAllConditionVariable_t pWakeAllConditionVariable =
                                    (once_block_impl_nt6::WakeAllConditionVariable_t)GetProcAddress(hKernel32, "WakeAllConditionVariable");
                                if (pWakeAllConditionVariable)
                                {
                                    return new once_block_impl_nt6(
                                        pInitializeSRWLock,
                                        pAcquireSRWLockExclusive,
                                        pReleaseSRWLockExclusive,
                                        pInitializeConditionVariable,
                                        pSleepConditionVariableSRW,
                                        pWakeAllConditionVariable);
                                }
                            }
                        }
                    }
                }
            }
        }

        return new once_block_impl_nt5();
    }

    once_block_impl_base* g_pOnceBlockImpl = NULL;

    void destroy_once_block_impl()
    {
        once_block_impl_base* impl = (once_block_impl_base*)
            BOOST_INTERLOCKED_EXCHANGE_POINTER((void**)&g_pOnceBlockImpl, NULL);
        delete impl;
    }

    once_block_impl_base* get_once_block_impl() BOOST_NOEXCEPT
    {
        once_block_impl_base* impl = g_pOnceBlockImpl;
        if (!impl) try
        {
            once_block_impl_base* new_impl = create_once_block_impl();
            impl = (once_block_impl_base*)
                BOOST_INTERLOCKED_COMPARE_EXCHANGE_POINTER((void**)&g_pOnceBlockImpl, (void*)new_impl, NULL);
            if (impl)
            {
                delete new_impl;
            }
            else
            {
                std::atexit(&destroy_once_block_impl);
                return new_impl;
            }
        }
        catch (...)
        {
            BOOST_ASSERT_MSG(false, "Boost.Log: Failed to initialize the once block thread synchronization structures");
            std::abort();
        }

        return impl;
    }

} // namespace

BOOST_LOG_API bool once_block_sentry::enter_once_block() const BOOST_NOEXCEPT
{
    return get_once_block_impl()->enter_once_block(m_flag);
}

BOOST_LOG_API void once_block_sentry::commit() BOOST_NOEXCEPT
{
    get_once_block_impl()->commit(m_flag);
}

BOOST_LOG_API void once_block_sentry::rollback() BOOST_NOEXCEPT
{
    get_once_block_impl()->rollback(m_flag);
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // defined(BOOST_LOG_USE_WINNT6_API)

#elif defined(BOOST_THREAD_PLATFORM_PTHREAD)

#include <pthread.h>
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

BOOST_LOG_ANONYMOUS_NAMESPACE {

static pthread_mutex_t g_OnceBlockMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_OnceBlockCond = PTHREAD_COND_INITIALIZER;

} // namespace

BOOST_LOG_API bool once_block_sentry::enter_once_block() const BOOST_NOEXCEPT
{
    BOOST_VERIFY(!pthread_mutex_lock(&g_OnceBlockMutex));

    once_block_flag volatile& flag = m_flag;
    while (flag.status != once_block_flag::initialized)
    {
        if (flag.status == once_block_flag::uninitialized)
        {
            flag.status = once_block_flag::being_initialized;
            BOOST_VERIFY(!pthread_mutex_unlock(&g_OnceBlockMutex));

            // Invoke the initializer block
            return false;
        }
        else
        {
            while (flag.status == once_block_flag::being_initialized)
            {
                BOOST_VERIFY(!pthread_cond_wait(&g_OnceBlockCond, &g_OnceBlockMutex));
            }
        }
    }

    BOOST_VERIFY(!pthread_mutex_unlock(&g_OnceBlockMutex));

    return true;
}

BOOST_LOG_API void once_block_sentry::commit() BOOST_NOEXCEPT
{
    BOOST_VERIFY(!pthread_mutex_lock(&g_OnceBlockMutex));

    // The initializer executed successfully
    m_flag.status = once_block_flag::initialized;

    BOOST_VERIFY(!pthread_mutex_unlock(&g_OnceBlockMutex));
    BOOST_VERIFY(!pthread_cond_broadcast(&g_OnceBlockCond));
}

BOOST_LOG_API void once_block_sentry::rollback() BOOST_NOEXCEPT
{
    BOOST_VERIFY(!pthread_mutex_lock(&g_OnceBlockMutex));

    // The initializer failed, marking the flag as if it hasn't run at all
    m_flag.status = once_block_flag::uninitialized;

    BOOST_VERIFY(!pthread_mutex_unlock(&g_OnceBlockMutex));
    BOOST_VERIFY(!pthread_cond_broadcast(&g_OnceBlockCond));
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#else
#error Boost.Log: unsupported threading API
#endif

#endif // BOOST_LOG_NO_THREADS
