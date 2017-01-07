#include "memory_utils.hpp"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef _WIN32
#include "windows.hpp"
#include <io.h>     // NOLINT
#include <direct.h> // NOLINT
#ifndef __MINGW32__
#include <filesystem>
#endif
#endif  // _WIN32

#include "errors.hpp"

void *raw_malloc_aligned(size_t size, size_t alignment) {
    void *ptr = nullptr;
#ifdef _WIN32
    ptr = _aligned_malloc(size, alignment);
    if (UNLIKELY(ptr == nullptr)) {
        crash_oom();
    }
#else
    int res = posix_memalign(&ptr, alignment, size);  // NOLINT(runtime/rethinkdb_fn)
    if (res != 0) {
        if (res == EINVAL) {
            crash_or_trap("posix_memalign with bad alignment: %zu.", alignment);
        } else if (res == ENOMEM) {
            crash_oom();
        } else {
            crash_or_trap("posix_memalign failed with unknown result: %d.", res);
        }
    }
#endif
    return ptr;
}

#ifndef _WIN32
void *raw_malloc_page_aligned(size_t size) {
    return raw_malloc_aligned(size, getpagesize());
}
#endif

void raw_free_aligned(void *ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

void *rmalloc(size_t size) {
    void *res = malloc(size);  // NOLINT(runtime/rethinkdb_fn)
    if (UNLIKELY(res == nullptr && size != 0)) {
        crash_oom();
    }
    return res;
}

void *rrealloc(void *ptr, size_t size) {
    void *res = realloc(ptr, size);  // NOLINT(runtime/rethinkdb_fn)
    if (UNLIKELY(res == nullptr && size != 0)) {
        crash_oom();
    }
    return res;
}
