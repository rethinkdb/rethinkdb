//  directory_management.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_DIRECTORY_MANAGEMENT_HPP
#define BOOST_DETAIL_WINAPI_DIRECTORY_MANAGEMENT_HPP

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
    using ::CreateDirectory;
    using ::CreateDirectoryA;
    using ::GetTempPathA;
    using ::RemoveDirectoryA;
#else
extern "C" { 
    __declspec(dllimport) int __stdcall 
        CreateDirectory(LPCTSTR_, LPSECURITY_ATTRIBUTES_*);
    __declspec(dllimport) int __stdcall 
        CreateDirectoryA(LPCTSTR_, interprocess_security_attributes*);
    __declspec(dllimport) int __stdcall 
        GetTempPathA(unsigned long length, char *buffer);
    __declspec(dllimport) int __stdcall 
        RemoveDirectoryA(LPCTSTR_);
}    
#endif
}
}
}

#endif // BOOST_DETAIL_WINAPI_THREAD_HPP
