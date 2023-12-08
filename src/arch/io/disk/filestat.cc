#include "arch/io/disk/filestat.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "errors.hpp"

int64_t get_file_size(fd_t fd) {
    guarantee(fd != INVALID_FD);

#ifdef _WIN32
    ULARGE_INTEGER size;
    size.LowPart = GetFileSize(fd, &size.HighPart);
    guarantee(size.LowPart != INVALID_FILE_SIZE, "GetFileSize failed");
    return size.QuadPart;
#else
    // No fstat64 -- we pass -D_FILE_OFFSET_BITS=64, musl dropped fstat64, and the static
    // assertions below check for 64 bit sizes.

    struct stat buf;
    int res = fstat(fd, &buf);

    // This compile-time assertion is the most important line in the function.
    CT_ASSERT(sizeof(buf.st_size) == sizeof(int64_t));

    // This one also sanity-checks we aren't in any non-_FILE_OFFSET_BITS==64-type regime.
    static_assert(sizeof(off_t) == sizeof(int64_t), "expecting off_t to be 64 bits");

    guarantee_err(res == 0, "fstat failed");
    guarantee(buf.st_size >= 0);
    return buf.st_size;
#endif
}
