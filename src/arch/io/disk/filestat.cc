#include "arch/io/disk/filestat.hpp"

#include <sys/stat.h>

#include "errors.hpp"

int64_t get_file_size(int fd) {
    guarantee(fd != -1);

#if defined(__MACH__) || defined(__sun)
    struct stat buf;
    int res = fstat(fd, &buf);
#else
    struct stat64 buf;
    int res = fstat64(fd, &buf);
#endif
    // This compile-time assertion is the most important line in the function.
    CT_ASSERT(sizeof(buf.st_size) == sizeof(int64_t));

    guarantee_err(res == 0, "fstat failed");
    guarantee(buf.st_size >= 0);
    return buf.st_size;
}
