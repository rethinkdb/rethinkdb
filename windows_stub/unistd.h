#pragma once

// TODO ATN

#include "windows.hpp"
#include <io.h>
#include <process.h>
#include <sys/types.h>
#include <stdio.h>
#include <BaseTsd.h>

#include "errors.hpp"
#include "logger.hpp"

typedef int gid_t;
typedef int uid_t;

// TODO ATN: get rid of references to pid_t
#ifndef __MINGW32__
typedef DWORD pid_t;
#endif

inline ssize_t pread(HANDLE h, void* buf, size_t count, off_t offset) {
    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.Offset = offset & 0xFFFFFFFF;
    // overlapped.OffsetHigh = offset >> 32; // ATN TODO
    DWORD nb_bytes;
    if (ReadFile(h, buf, count, &nb_bytes, &overlapped) == FALSE) {
        set_errno(EIO); // TODO ATN: GetLastError -> errno
        logERR("pread: ReadFile failed: %s\n", winerr_string(GetLastError()).c_str());
        return -1;
    }
    return nb_bytes;
}

inline ssize_t pwrite(HANDLE h, const void *buf, size_t count, off_t offset) {
    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.Offset = offset & 0xFFFFFFFF;
    // overlapped.OffsetHigh = offset >> 32; // ATN TODO
    DWORD bytes_written;
    if (WriteFile(h, buf, count, &bytes_written, &overlapped) == FALSE) {
        DebugBreak();
        set_errno(EIO); // TODO ATN: GetLastError -> errno
        logERR("pwrite: WriteFile failed: %s", winerr_string(GetLastError()).c_str());
        return -1;
    }
    return bytes_written;
}

#ifdef _MSC_VER
// for access
#define F_OK 00
#define W_OK 02
#define R_OK 04
#endif

#define close _close
#define getpid _getpid
