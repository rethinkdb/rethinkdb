#pragma once

// RSI ATN

#include "windows.hpp"
#include <io.h>
#include <process.h>
#include <sys/types.h>
#include <stdio.h>
#include <BaseTsd.h>

typedef int gid_t;
typedef int uid_t;
typedef DWORD pid_t;

inline ssize_t pread(HANDLE h, void* buf, size_t count, off_t offset) {
    if (SetFilePointer(h, count, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        rassert("ATN TODO: errno -- GetLastError");
        return -1;
    }
    DWORD nb_bytes;
    if (ReadFile(h, buf, count, &nb_bytes, NULL) == FALSE) {
        rassert("ATN TODO: errno -- GetLastError");
        return -1;
    }
    return nb_bytes;
}

inline ssize_t pwrite(HANDLE h, const void *buf, size_t count, off_t offset) {
    if (SetFilePointer(h, count, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        rassert("ATN TODO: errno -- GetLastError");
        return -1;
    }
    DWORD bytes_written;
    if (WriteFile(h, buf, count, &bytes_written, NULL) == FALSE) {
        rassert("ATN TODO: errno -- GetLastError");
        return -1;
    }
    return bytes_written;
}

// for access
#define F_OK 00
#define W_OK 02
#define R_OK 04

#define close _close
#define getpid _getpid
