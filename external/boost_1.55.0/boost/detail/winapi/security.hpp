//  security.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_SECURITY_HPP
#define BOOST_DETAIL_WINAPI_SECURITY_HPP

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
typedef ::SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES_;
typedef ::PSECURITY_ATTRIBUTES PSECURITY_ATTRIBUTES_;
typedef ::LPSECURITY_ATTRIBUTES LPSECURITY_ATTRIBUTES_;
    
#else
extern "C" { 
    struct SECURITY_DESCRIPTOR_;
    typedef SECURITY_DESCRIPTOR_* PSECURITY_DESCRIPTOR_;
    typedef struct _ACL {
      BYTE_ AclRevision;
      BYTE_ Sbz1;
      WORD_ AclSize;
      WORD_ AceCount;
      WORD_ Sbz2;
    } ACL_, *PACL_;

    typedef struct _SECURITY_ATTRIBUTES {
      DWORD_  nLength;
      LPVOID_ lpSecurityDescriptor;
      BOOL_   bInheritHandle;
    } SECURITY_ATTRIBUTES_, *PSECURITY_ATTRIBUTES_, *LPSECURITY_ATTRIBUTES_;

    __declspec(dllimport) BOOL_ __stdcall 
        InitializeSecurityDescriptor(
            PSECURITY_DESCRIPTOR_ pSecurityDescriptor,
            DWORD_ dwRevision
    );
    __declspec(dllimport) BOOL_ __stdcall 
        SetSecurityDescriptorDacl(
            PSECURITY_DESCRIPTOR_ pSecurityDescriptor,
            BOOL_ bDaclPresent,
            PACL_ pDacl,
            BOOL_ bDaclDefaulted
    );
}
#endif
}
}
}

#endif // BOOST_DETAIL_WINAPI_SECURITY_HPP
