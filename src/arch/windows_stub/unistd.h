#ifndef ARCH_WINDOWS_STUB_UNISTD_H_
#define ARCH_WINDOWS_STUB_UNISTD_H_

// A stub for <unistd.h> on windows

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

inline ssize_t pread(HANDLE h, void* buf, size_t count, off_t offset) {
    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.Offset = offset & 0xFFFFFFFF;
    // WINDOWS TODO
    // overlapped.OffsetHigh = offset >> 32;
    DWORD nb_bytes;
    if (ReadFile(h, buf, count, &nb_bytes, &overlapped) == FALSE) {
        set_errno(EIO);
        logERR("pread: ReadFile failed: %s\n", winerr_string(GetLastError()).c_str());
        return -1;
    }
    return nb_bytes;
}

inline ssize_t pwrite(HANDLE h, const void *buf, size_t count, off_t offset) {
    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.Offset = offset & 0xFFFFFFFF;
    // WINDOWS TODO
    // overlapped.OffsetHigh = offset >> 32;
    DWORD bytes_written;
    if (WriteFile(h, buf, count, &bytes_written, &overlapped) == FALSE) {
        set_errno(EIO);
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

#endif
