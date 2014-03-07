//  thread.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_THREAD_HPP
#define BOOST_DETAIL_WINAPI_THREAD_HPP

#include <boost/detail/winapi/basic_types.hpp>
#include <boost/detail/winapi/GetCurrentThread.hpp>

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
    using ::GetCurrentThreadId;
    using ::SleepEx;
    using ::Sleep;
#else
extern "C" { 
# ifndef UNDER_CE
    __declspec(dllimport) unsigned long __stdcall 
        GetCurrentThreadId(void);
    __declspec(dllimport) unsigned long __stdcall 
        SleepEx(unsigned long,int);
    __declspec(dllimport) void __stdcall 
        Sleep(unsigned long);
#else
    using ::GetCurrentThreadId;
    using ::SleepEx;
    using ::Sleep;
#endif    
}    
#endif
}
}
}

#endif // BOOST_DETAIL_WINAPI_THREAD_HPP
