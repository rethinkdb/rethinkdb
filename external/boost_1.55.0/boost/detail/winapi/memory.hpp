//  memory.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_MEMORY_HPP
#define BOOST_DETAIL_WINAPI_MEMORY_HPP

#include <boost/detail/winapi/basic_types.hpp>
#include <boost/detail/winapi/security.hpp>
#include <boost/detail/winapi/LocalFree.hpp>

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
    using ::CreateFileMappingA;
    using ::FlushViewOfFile;
    using ::GetProcessHeap;
    using ::HeapAlloc;
    using ::HeapFree;
    using ::MapViewOfFileEx;
    using ::OpenFileMappingA;
    using ::UnmapViewOfFile;
#else
#undef HeapAlloc
extern "C" {
    __declspec(dllimport) void * __stdcall 
        CreateFileMappingA (void *, SECURITY_ATTRIBUTES_*, unsigned long, unsigned long, unsigned long, const char *);
    __declspec(dllimport) int __stdcall 
        FlushViewOfFile (void *, std::size_t);
    __declspec(dllimport) HANDLE_ __stdcall 
        GetProcessHeap();
    __declspec(dllimport) void* __stdcall 
        HeapAlloc(HANDLE_,DWORD_,SIZE_T_);
    __declspec(dllimport) BOOL_ __stdcall 
        HeapFree(HANDLE_,DWORD_,LPVOID_);
    __declspec(dllimport) void * __stdcall 
        MapViewOfFileEx (void *, unsigned long, unsigned long, unsigned long, std::size_t, void*);
    __declspec(dllimport) void * __stdcall 
        OpenFileMappingA (unsigned long, int, const char *);
    __declspec(dllimport) int __stdcall 
        UnmapViewOfFile(void *);
}
#endif
}
}
}

#endif // BOOST_DETAIL_WINAPI_MEMORY_HPP
