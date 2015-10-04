#pragma once

// TODO ATN

#include "windows.hpp"
#include <io.h>
#include <process.h>
#include <sys/types.h>
#include <stdio.h>
#include <BaseTsd.h>

#include "errors.hpp"

typedef int gid_t;
typedef int uid_t;

// TODO ATN: get rid of references to pid_t
#ifndef __MINGW32__
typedef DWORD pid_t;
#endif

inline ssize_t pread(HANDLE h, void* buf, size_t count, off_t offset) {
    DWORD res = SetFilePointer(h, offset, NULL, FILE_BEGIN);
    if (res == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        set_errno(EIO); // TODO ATN: GetLastError -> errno
        fprintf(stderr, "pread: SetFilePointer failed: %s", winerr_string(GetLastError()).c_str());
        return -1;
    }
    DWORD nb_bytes;
    if (ReadFile(h, buf, count, &nb_bytes, NULL) == FALSE) {
        set_errno(EIO); // TODO ATN: GetLastError -> errno
        fprintf(stderr, "pread: ReadFile failed: %s", winerr_string(GetLastError()).c_str());
        return -1;
    }
    return nb_bytes;
}

inline ssize_t pwrite(HANDLE h, const void *buf, size_t count, off_t offset) {
    if (SetFilePointer(h, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        set_errno(EIO); // TODO ATN: GetLastError -> errno
        fprintf(stderr, "pwrite: SetFilePointer failed: %s", winerr_string(GetLastError()).c_str());
        return -1;
    }
    DWORD bytes_written;
    if (WriteFile(h, buf, count, &bytes_written, NULL) == FALSE) {
        set_errno(EIO); // TODO ATN: GetLastError -> errno
        fprintf(stderr, "pwrite: WriteFile failed: %s", winerr_string(GetLastError()).c_str());
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
