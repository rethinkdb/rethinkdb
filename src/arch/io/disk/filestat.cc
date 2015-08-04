#include "arch/io/disk/filestat.hpp"

#include <sys/stat.h>

#include "errors.hpp"
#include "arch/runtime/runtime_utils.hpp"

int64_t get_file_size(fd_t fd) {
    guarantee(fd != INVALID_FD);

#ifdef _WIN32
    ULARGE_INTEGER size;
    size.LowPart = GetFileSize(fd, &size.HighPart);
    guarantee(size.LowPart != INVALID_FILE_SIZE, "GetFileSize failed");
    return size.QuadPart;
#else
#ifdef __MACH__
    struct stat buf;
    int res = fstat(fd, &buf);
#elif defined(__linux__)
    struct stat64 buf;
    int res = fstat64(fd, &buf);
#endif
    // This compile-time assertion is the most important line in the function.
    CT_ASSERT(sizeof(buf.st_size) == sizeof(int64_t));

    guarantee_err(res == 0, "fstat failed");
    guarantee(buf.st_size >= 0);
    return buf.st_size;
#endif
}
