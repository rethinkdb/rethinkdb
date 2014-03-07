//  system.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_SYSTEM_HPP
#define BOOST_DETAIL_WINAPI_SYSTEM_HPP

#include <boost/detail/winapi/basic_types.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace detail {
namespace winapi {
#if defined( BOOST_USE_WINDOWS_H )
    typedef ::SYSTEM_INFO SYSTEM_INFO_;
    extern "C" __declspec(dllimport) void __stdcall GetSystemInfo (struct system_info *);
#else
extern "C" {
    typedef struct _SYSTEM_INFO {
      union {
        DWORD_  dwOemId;
        struct {
          WORD_ wProcessorArchitecture;
          WORD_ wReserved;
        } dummy;
      } ;
      DWORD_     dwPageSize;
      LPVOID_    lpMinimumApplicationAddress;
      LPVOID_    lpMaximumApplicationAddress;
      DWORD_PTR_ dwActiveProcessorMask;
      DWORD_     dwNumberOfProcessors;
      DWORD_     dwProcessorType;
      DWORD_     dwAllocationGranularity;
      WORD_      wProcessorLevel;
      WORD_      wProcessorRevision;
    } SYSTEM_INFO_;

    __declspec(dllimport) void __stdcall 
        GetSystemInfo (struct system_info *);
}    
#endif
}
}
}
#endif // BOOST_DETAIL_WINAPI_SYSTEM_HPP
