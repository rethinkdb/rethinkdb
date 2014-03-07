/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   thread_specific.cpp
 * \author Andrey Semashev
 * \date   01.03.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#include <string>
#include <stdexcept>
#include <boost/log/exceptions.hpp>
#include <boost/log/detail/thread_specific.hpp>

#if !defined(BOOST_LOG_NO_THREADS)

#include <boost/thread/tss.hpp> // To hook on Boost.Thread configuration macros
#include <boost/log/detail/header.hpp>


#if defined(BOOST_THREAD_PLATFORM_WIN32)

#include "windows_version.hpp"
#include <windows.h>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

thread_specific_base::thread_specific_base()
{
    m_Key.as_dword = TlsAlloc();
    if (m_Key.as_dword == TLS_OUT_OF_INDEXES)
    {
        BOOST_LOG_THROW_DESCR(system_error, "TLS capacity depleted");
    }
    set_content(0);
}

thread_specific_base::~thread_specific_base()
{
    TlsFree(m_Key.as_dword);
}

void* thread_specific_base::get_content() const
{
    return TlsGetValue(m_Key.as_dword);
}

void thread_specific_base::set_content(void* value) const
{
    TlsSetValue(m_Key.as_dword, value);
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#elif defined(BOOST_THREAD_PLATFORM_PTHREAD)

#include <pthread.h>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/alignment_of.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

BOOST_LOG_ANONYMOUS_NAMESPACE {

    //! A helper template to disable early name binding
    template< typename NonDependentT, typename DependentT >
    struct make_dependent
    {
        typedef NonDependentT type;
    };

    //! Some portability magic to detect where to store the TLS key
    template<
        typename StorageT,
        bool IsStoreableV = sizeof(pthread_key_t) <= sizeof(StorageT)
            && alignment_of< pthread_key_t >::value <= alignment_of< StorageT >::value,
        bool IsPointerV = is_pointer< pthread_key_t >::value
    >
    struct pthread_key_traits;

    //! Worst case - the key is probably some structure
    template< typename StorageT, bool IsPointerV >
    struct pthread_key_traits< StorageT, false, IsPointerV >
    {
        typedef typename make_dependent< pthread_key_t, StorageT >::type pthread_key_type;

        static void allocate(StorageT& stg)
        {
            pthread_key_type* pkey = new pthread_key_type;
            if (pthread_key_create(pkey, 0) != 0)
            {
                delete pkey;
                BOOST_LOG_THROW_DESCR(system_error, "TLS capacity depleted");
            }
            stg.as_pointer = pkey;
        }

        static void deallocate(StorageT& stg)
        {
            pthread_key_type* pkey = static_cast< pthread_key_type* >(stg.as_pointer);
            pthread_key_delete(*pkey);
            delete pkey;
        }

        static void set_value(StorageT const& stg, void* value)
        {
            pthread_setspecific(*static_cast< pthread_key_type* >(stg.as_pointer), value);
        }

        static void* get_value(StorageT const& stg)
        {
            return pthread_getspecific(*static_cast< pthread_key_type* >(stg.as_pointer));
        }
    };

    //! The key is a pointer
    template< typename StorageT >
    struct pthread_key_traits< StorageT, true, true >
    {
        typedef typename make_dependent< pthread_key_t, StorageT >::type pthread_key_type;

        static void allocate(StorageT& stg)
        {
            if (pthread_key_create(reinterpret_cast< pthread_key_type* >(&stg.as_pointer), 0) != 0)
            {
                BOOST_LOG_THROW_DESCR(system_error, "TLS capacity depleted");
            }
        }

        static void deallocate(StorageT& stg)
        {
            pthread_key_delete(reinterpret_cast< pthread_key_type >(stg.as_pointer));
        }

        static void set_value(StorageT const& stg, void* value)
        {
            pthread_setspecific(reinterpret_cast< const pthread_key_type >(stg.as_pointer), value);
        }

        static void* get_value(StorageT const& stg)
        {
            return pthread_getspecific(reinterpret_cast< const pthread_key_type >(stg.as_pointer));
        }
    };

    //! The most probable case - the key is an integral or a structure that contains one
    template< typename StorageT >
    struct pthread_key_traits< StorageT, true, false >
    {
        typedef typename make_dependent< pthread_key_t, StorageT >::type pthread_key_type;

        static void allocate(StorageT& stg)
        {
            if (pthread_key_create(reinterpret_cast< pthread_key_type* >(&stg.as_dword), 0) != 0)
            {
                BOOST_LOG_THROW_DESCR(system_error, "TLS capacity depleted");
            }
        }

        static void deallocate(StorageT& stg)
        {
            pthread_key_delete(*reinterpret_cast< pthread_key_type* >(&stg.as_dword));
        }

        static void set_value(StorageT const& stg, void* value)
        {
            pthread_setspecific(*reinterpret_cast< pthread_key_type const* >(&stg.as_dword), value);
        }

        static void* get_value(StorageT const& stg)
        {
            return pthread_getspecific(*reinterpret_cast< pthread_key_type const* >(&stg.as_dword));
        }
    };

} // namespace

thread_specific_base::thread_specific_base()
{
    typedef pthread_key_traits< key_storage > traits_t;
    traits_t::allocate(m_Key);
    set_content(0);
}

thread_specific_base::~thread_specific_base()
{
    typedef pthread_key_traits< key_storage > traits_t;
    traits_t::deallocate(m_Key);
}

void* thread_specific_base::get_content() const
{
    typedef pthread_key_traits< key_storage > traits_t;
    return traits_t::get_value(m_Key);
}

void thread_specific_base::set_content(void* value) const
{
    typedef pthread_key_traits< key_storage > traits_t;
    traits_t::set_value(m_Key, value);
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#else
#error Boost.Log: unsupported threading API
#endif

#include <boost/log/detail/footer.hpp>

#endif // !defined(BOOST_LOG_NO_THREADS)
