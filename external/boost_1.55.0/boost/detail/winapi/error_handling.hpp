//  error_handling.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_ERROR_HANDLING_HPP
#define BOOST_DETAIL_WINAPI_ERROR_HANDLING_HPP

#include <boost/detail/winapi/basic_types.hpp>
#include <boost/detail/winapi/GetCurrentThread.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace detail {
namespace winapi {

#if defined( BOOST_USE_WINDOWS_H )
    using ::FormatMessageA;
    using ::FormatMessageW;

    const int FORMAT_MESSAGE_ALLOCATE_BUFFER_= FORMAT_MESSAGE_ALLOCATE_BUFFER;
    const int FORMAT_MESSAGE_IGNORE_INSERTS_=  FORMAT_MESSAGE_IGNORE_INSERTS;
    const int FORMAT_MESSAGE_FROM_STRING_=     FORMAT_MESSAGE_FROM_STRING;
    const int FORMAT_MESSAGE_FROM_HMODULE_=    FORMAT_MESSAGE_FROM_HMODULE;
    const int FORMAT_MESSAGE_FROM_SYSTEM_=     FORMAT_MESSAGE_FROM_SYSTEM;
    const int FORMAT_MESSAGE_ARGUMENT_ARRAY_=  FORMAT_MESSAGE_ARGUMENT_ARRAY;
    const int FORMAT_MESSAGE_MAX_WIDTH_MASK_=  FORMAT_MESSAGE_MAX_WIDTH_MASK;

    const char LANG_NEUTRAL_=                  LANG_NEUTRAL;
    const char LANG_INVARIANT_=                LANG_INVARIANT;

    const char SUBLANG_DEFAULT_=               SUBLANG_DEFAULT;    // user default
    inline WORD_ MAKELANGID_(WORD_ p, WORD_ s) {
        return MAKELANGID(p,s);
    }
#else
extern "C" {
    //                using ::FormatMessageA;
    __declspec(dllimport)
    DWORD_
    WINAPI
    FormatMessageA(
        DWORD_ dwFlags,
        LPCVOID_ lpSource,
        DWORD_ dwMessageId,
        DWORD_ dwLanguageId,
        LPSTR_ lpBuffer,
        DWORD_ nSize,
        va_list *Arguments
        );

    //                using ::FormatMessageW;
    __declspec(dllimport)
    DWORD_
    WINAPI
    FormatMessageW(
        DWORD_ dwFlags,
        LPCVOID_ lpSource,
        DWORD_ dwMessageId,
        DWORD_ dwLanguageId,
        LPWSTR_ lpBuffer,
        DWORD_ nSize,
        va_list *Arguments
        );

    const int FORMAT_MESSAGE_ALLOCATE_BUFFER_= 0x00000100;
    const int FORMAT_MESSAGE_IGNORE_INSERTS_=  0x00000200;
    const int FORMAT_MESSAGE_FROM_STRING_=     0x00000400;
    const int FORMAT_MESSAGE_FROM_HMODULE_=    0x00000800;
    const int FORMAT_MESSAGE_FROM_SYSTEM_=     0x00001000;
    const int FORMAT_MESSAGE_ARGUMENT_ARRAY_=  0x00002000;
    const int FORMAT_MESSAGE_MAX_WIDTH_MASK_=  0x000000FF;

    const char LANG_NEUTRAL_=                  0x00;
    const char LANG_INVARIANT_=                0x7f;

    const char SUBLANG_DEFAULT_=               0x01;    // user default
    inline WORD_ MAKELANGID_(WORD_ p, WORD_ s) {
        return ((((WORD_  )(s)) << 10) | (WORD_  )(p));
    }

}
#endif
}
}
}
#endif // BOOST_DETAIL_WINAPI_ERROR_HANDLING_HPP
