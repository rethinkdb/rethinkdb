//  dll.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_DLL_HPP
#define BOOST_DETAIL_WINAPI_DLL_HPP

#include <boost/detail/winapi/basic_types.hpp>
#include <boost/detail/winapi/security.hpp>

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
    using ::LoadLibrary;
    using ::FreeLibrary;
    using ::GetProcAddress;
    using ::GetModuleHandleA;
#else
extern "C" { 
    __declspec(dllimport) HMODULE_ __stdcall 
        LoadLibrary(
            LPCTSTR_ lpFileName
    );
    __declspec(dllimport) BOOL_ __stdcall 
        FreeLibrary(
            HMODULE_ hModule
    );
    __declspec(dllimport) FARPROC_ __stdcall 
        GetProcAddress(
            HMODULE_ hModule,
            LPCSTR_ lpProcName
    );
    __declspec(dllimport) FARPROC_ __stdcall 
        GetModuleHandleA(
            LPCSTR_ lpProcName
    );
}    
#endif
}
}
}

#endif // BOOST_DETAIL_WINAPI_THREAD_HPP
