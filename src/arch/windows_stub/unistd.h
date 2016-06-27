#ifndef SRC_ARCH_WINDOWS_STUB_UNISTD_H_
#define SRC_ARCH_WINDOWS_STUB_UNISTD_H_

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

// Posix uses `off_t` for `offset`, but `off_t` is 32 bit on Windows so that's no good.
// We diverge from it and use `int64_t` instead.
inline ssize_t pread(HANDLE h, void* buf, size_t count, int64_t offset) {
    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    guarantee(offset >= 0);
    uint64_t unsigned_offset = offset;
    overlapped.Offset = unsigned_offset & 0xFFFFFFFF;
    overlapped.OffsetHigh = unsigned_offset >> 32;
    DWORD nb_bytes;
    if (ReadFile(h, buf, count, &nb_bytes, &overlapped) == FALSE) {
        set_errno(EIO);
        logERR("pread: ReadFile failed: %s\n", winerr_string(GetLastError()).c_str());
        return -1;
    }
    return nb_bytes;
}

// See comment above about the `int64_t offset`.
inline ssize_t pwrite(HANDLE h, const void *buf, size_t count, int64_t offset) {
    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    guarantee(offset >= 0);
    uint64_t unsigned_offset = offset;
    overlapped.Offset = unsigned_offset & 0xFFFFFFFF;
    overlapped.OffsetHigh = unsigned_offset >> 32;
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

#endif  // SRC_ARCH_WINDOWS_STUB_UNISTD_H_
