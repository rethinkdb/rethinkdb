//  synchronizaion.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_SYNCHRONIZATION_HPP
#define BOOST_DETAIL_WINAPI_SYNCHRONIZATION_HPP

#include <boost/detail/winapi/basic_types.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost
{
namespace detail
{
namespace winapi
{
#if defined( BOOST_USE_WINDOWS_H )
    typedef ::CRITICAL_SECTION CRITICAL_SECTION_;
    typedef ::PAPCFUNC PAPCFUNC_;

    using ::InitializeCriticalSection;
    using ::EnterCriticalSection;
    using ::TryEnterCriticalSection;
    using ::LeaveCriticalSection;
    using ::DeleteCriticalSection;

# ifdef BOOST_NO_ANSI_APIS
    using ::CreateMutexW;
    using ::CreateEventW;
    using ::OpenEventW;
    using ::CreateSemaphoreW;
# else
    using ::CreateMutexA;
    using ::CreateEventA;
    using ::OpenEventA;
    using ::CreateSemaphoreA;
# endif
    using ::ReleaseMutex;
    using ::ReleaseSemaphore;
    using ::SetEvent;
    using ::ResetEvent;
    using ::WaitForMultipleObjects;
    using ::WaitForSingleObject;
    using ::QueueUserAPC;

    const DWORD_ infinite       = INFINITE;
    const DWORD_ wait_abandoned = WAIT_ABANDONED;
    const DWORD_ wait_object_0  = WAIT_OBJECT_0;
    const DWORD_ wait_timeout   = WAIT_TIMEOUT;
    const DWORD_ wait_failed    = WAIT_FAILED;

#else // defined( BOOST_USE_WINDOWS_H )

extern "C" {

    struct CRITICAL_SECTION_
    {
        struct critical_section_debug * DebugInfo;
        long LockCount;
        long RecursionCount;
        void * OwningThread;
        void * LockSemaphore;
    #if defined(_WIN64)
        unsigned __int64 SpinCount;
    #else
        unsigned long SpinCount;
    #endif
    };

     __declspec(dllimport) void __stdcall
        InitializeCriticalSection(CRITICAL_SECTION_ *);
    __declspec(dllimport) void __stdcall
        EnterCriticalSection(CRITICAL_SECTION_ *);
    __declspec(dllimport) bool __stdcall
        TryEnterCriticalSection(CRITICAL_SECTION_ *);
    __declspec(dllimport) void __stdcall
        LeaveCriticalSection(CRITICAL_SECTION_ *);
    __declspec(dllimport) void __stdcall
        DeleteCriticalSection(CRITICAL_SECTION_ *);

    struct _SECURITY_ATTRIBUTES;
# ifdef BOOST_NO_ANSI_APIS
    __declspec(dllimport) void* __stdcall
        CreateMutexW(_SECURITY_ATTRIBUTES*,int,wchar_t const*);
    __declspec(dllimport) void* __stdcall
        CreateSemaphoreW(_SECURITY_ATTRIBUTES*,long,long,wchar_t const*);
    __declspec(dllimport) void* __stdcall
        CreateEventW(_SECURITY_ATTRIBUTES*,int,int,wchar_t const*);
    __declspec(dllimport) void* __stdcall
        OpenEventW(unsigned long,int,wchar_t const*);
# else
    __declspec(dllimport) void* __stdcall
        CreateMutexA(_SECURITY_ATTRIBUTES*,int,char const*);
    __declspec(dllimport) void* __stdcall
        CreateSemaphoreA(_SECURITY_ATTRIBUTES*,long,long,char const*);
    __declspec(dllimport) void* __stdcall
        CreateEventA(_SECURITY_ATTRIBUTES*,int,int,char const*);
    __declspec(dllimport) void* __stdcall
        OpenEventA(unsigned long,int,char const*);
# endif
    __declspec(dllimport) int __stdcall
        ReleaseMutex(void*);
    __declspec(dllimport) unsigned long __stdcall
        WaitForSingleObject(void*,unsigned long);
    __declspec(dllimport) unsigned long __stdcall
        WaitForMultipleObjects(unsigned long nCount,
                void* const * lpHandles,
                int bWaitAll,
                unsigned long dwMilliseconds);
    __declspec(dllimport) int __stdcall
        ReleaseSemaphore(void*,long,long*);
    typedef void (__stdcall *PAPCFUNC8)(ULONG_PTR_);
    __declspec(dllimport) unsigned long __stdcall
        QueueUserAPC(PAPCFUNC8,void*,ULONG_PTR_);
# ifndef UNDER_CE
    __declspec(dllimport) int __stdcall
        SetEvent(void*);
    __declspec(dllimport) int __stdcall
        ResetEvent(void*);
# else
    using ::SetEvent;
    using ::ResetEvent;
# endif

} // extern "C"

    const DWORD_ infinite       = (DWORD_)0xFFFFFFFF;
    const DWORD_ wait_abandoned = 0x00000080L;
    const DWORD_ wait_object_0  = 0x00000000L;
    const DWORD_ wait_timeout   = 0x00000102L;
    const DWORD_ wait_failed    = (DWORD_)0xFFFFFFFF;

#endif // defined( BOOST_USE_WINDOWS_H )

}
}
}

#endif // BOOST_DETAIL_WINAPI_SYNCHRONIZATION_HPP
