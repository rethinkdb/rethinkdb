//  thread.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_FILE_MANAGEMENT_HPP
#define BOOST_DETAIL_WINAPI_FILE_MANAGEMENT_HPP

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
    using ::CreateFileA;
    using ::DeleteFileA;
    using ::FindFirstFileA;
    using ::FindNextFileA;
    using ::FindClose;
    using ::GetFileSizeEx;
    using ::MoveFileExA;
    using ::SetFileValidData;
#else
extern "C" {
    typedef struct _OVERLAPPED {
      ULONG_PTR Internal;
      ULONG_PTR InternalHigh;
      union {
        struct {
          DWORD Offset;
          DWORD OffsetHigh;
        } ;
        PVOID  Pointer;
      } ;
      HANDLE    hEvent;
    } OVERLAPPED, *LPOVERLAPPED;

    
    __declspec(dllimport) void * __stdcall 
        CreateFileA (const char *, unsigned long, unsigned long, struct SECURITY_ATTRIBUTES_*, unsigned long, unsigned long, void *);
    __declspec(dllimport) int __stdcall    
        DeleteFileA (const char *);
    __declspec(dllimport) void *__stdcall 
        FindFirstFileA(const char *lpFileName, win32_find_data_t *lpFindFileData);
    __declspec(dllimport) int   __stdcall 
        FindNextFileA(void *hFindFile, win32_find_data_t *lpFindFileData);
    __declspec(dllimport) int   __stdcall 
        FindClose(void *hFindFile);
    __declspec(dllimport) BOOL __stdcall 
        GetFileSizeEx(
            HANDLE_ hFile,
            PLARGE_INTEGER_ lpFileSize
    );
    __declspec(dllimport) int __stdcall    
        MoveFileExA (const char *, const char *, unsigned long);
    __declspec(dllimport) BOOL_ __stdcall 
        SetFileValidData(
            HANDLE_ hFile,
            LONGLONG_ ValidDataLength
    );
    __declspec(dllimport) BOOL_ __stdcall
        SetEndOfFile(
            HANDLE_ hFile
    );
    __declspec(dllimport) BOOL_ __stdcall
        SetFilePointerEx(
            HANDLE_ hFile,
            LARGE_INTEGER_ liDistanceToMove,
            PLARGE_INTEGER_ lpNewFilePointer,
            DWORD_ dwMoveMethod
    );
    __declspec(dllimport) BOOL_ __stdcall
        LockFile(
            HANDLE_ hFile,
            DWORD_ dwFileOffsetLow,
            DWORD_ dwFileOffsetHigh,
            DWORD_ nNumberOfBytesToLockLow,
            DWORD_ nNumberOfBytesToLockHigh
    );
    __declspec(dllimport) BOOL_ __stdcall
        UnlockFile(
            HANDLE_ hFile,
            DWORD_ dwFileOffsetLow,
            DWORD_ dwFileOffsetHigh,
            DWORD_ nNumberOfBytesToUnlockLow,
            DWORD_ nNumberOfBytesToUnlockHigh
    );
    __declspec(dllimport) BOOL_ __stdcall
        LockFileEx(
            HANDLE_ hFile,
            DWORD_ dwFlags,
            DWORD_ dwReserved,
            DWORD_ nNumberOfBytesToLockLow,
            DWORD_ nNumberOfBytesToLockHigh,
            LPOVERLAPPED_ lpOverlapped
    );
    __declspec(dllimport) BOOL_ __stdcall
        UnlockFileEx(
            HANDLE_ hFile,
            DWORD_ dwReserved,
            DWORD_ nNumberOfBytesToUnlockLow,
            DWORD_ nNumberOfBytesToUnlockHigh,
            LPOVERLAPPED_ lpOverlapped
    );
    __declspec(dllimport) BOOL_ __stdcall
        WriteFile(
            HANDLE_ hFile,
            LPCVOID_ lpBuffer,
            DWORD_ nNumberOfBytesToWrite,
            LPDWORD_ lpNumberOfBytesWritten,
            LPOVERLAPPED_ lpOverlapped
    );
}
#endif
}
}
}

#endif // BOOST_DETAIL_WINAPI_THREAD_HPP
