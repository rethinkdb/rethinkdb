#include "rethinkdb_backtrace.hpp"

#ifdef __MACH__

#include <execinfo.h>
#include <pthread.h>
#include <stdlib.h>

#include <algorithm>

#include "arch/runtime/coroutines.hpp"
#include "arch/runtime/context_switching.hpp"

struct pthread_t_field_locations_t {
    size_t stackaddr_offset;
    size_t stacksize_offset;
};

bool get_pthread_t_stack_field_locations(pthread_t th, pthread_t_field_locations_t *locations_out) {
    void *const stackaddr = pthread_get_stackaddr_np(th);
    const size_t stacksize = pthread_get_stacksize_np(th);

    pthread_t_field_locations_t locations;
    bool found_stackaddr = false;
    bool found_stacksize = false;

    const size_t step = std::min(sizeof(size_t), sizeof(void *));
    for (size_t i = 0; i < __PTHREAD_SIZE__; i += step) {
        char *const p = th->__opaque + i;
        if (!found_stackaddr
            && i <= __PTHREAD_SIZE__ - sizeof(void *)
            && *reinterpret_cast<void **>(p) == stackaddr) {
            void *const test_value = static_cast<char *>(stackaddr) + 8;
            *reinterpret_cast<void **>(p) = test_value;
            if (pthread_get_stackaddr_np(th) == test_value) {
                locations.stackaddr_offset = i;
                found_stackaddr = true;
            }
            *reinterpret_cast<void **>(p) = stackaddr;
        }

        if (!found_stacksize
            && i <= __PTHREAD_SIZE__ - sizeof(size_t)
            && *reinterpret_cast<size_t *>(p) == stacksize) {
            const size_t test_value = stacksize + 8;
            *reinterpret_cast<size_t *>(p) = test_value;
            if (pthread_get_stacksize_np(th) == test_value) {
                locations.stacksize_offset = i;
                found_stacksize = true;
            }
            *reinterpret_cast<size_t *>(p) = stacksize;
        }
    }

    if (found_stackaddr && found_stacksize) {
        *locations_out = locations;
        return true;
    } else {
        return false;
    }
}

void substitute_pthread_t_stack_fields(pthread_t th, pthread_t_field_locations_t locations,
                                       void *addr, size_t size) {
    *reinterpret_cast<void **>(th->__opaque + locations.stackaddr_offset) = addr;
    *reinterpret_cast<size_t *>(th->__opaque + locations.stacksize_offset) = size;
}

int rethinkdb_backtrace(void **buffer, int size) {
    coro_t *const coro = coro_t::self();
    if (coro == nullptr) {
        return backtrace(buffer, size);
    } else {
        pthread_t_field_locations_t field_locations;
        pthread_t self = pthread_self();
        if (!get_pthread_t_stack_field_locations(self, &field_locations)) {
            fprintf(stderr, "Could not retrieve pthread stack field locations.\n");
            return 0;
        }

        void *const stackaddr = pthread_get_stackaddr_np(self);
        const size_t stacksize = pthread_get_stacksize_np(self);

        coro_stack_t *const stack = coro->get_stack();
        void *const coro_addr = stack->get_stack_base();
        const size_t coro_size = static_cast<char *>(coro_addr)
            - static_cast<char *>(stack->get_stack_bound());

        substitute_pthread_t_stack_fields(self, field_locations, coro_addr, coro_size);
        const int res = backtrace(buffer, size);
        substitute_pthread_t_stack_fields(self, field_locations, stackaddr, stacksize);
        return res;
    }
}

#elif defined(_WIN32)

#include <algorithm>
#include <vector>

#include "windows.hpp"
#define OPTIONAL
#include <DbgHelp.h> // NOLINT

int rethinkdb_backtrace(void **buffer, int size) {
    std::vector<void *> addresses(size, nullptr);
    USHORT frames = CaptureStackBackTrace(0, size, addresses.data(), nullptr);
    if (frames > NUM_FRAMES_INSIDE_RETHINKDB_BACKTRACE) {
        std::move(addresses.begin() + NUM_FRAMES_INSIDE_RETHINKDB_BACKTRACE, addresses.begin() + frames, buffer);
        return frames - NUM_FRAMES_INSIDE_RETHINKDB_BACKTRACE;
    } else {
        return 0;
    }
}

#else

#include <execinfo.h>

#include "errors.hpp"

int rethinkdb_backtrace(void **buffer, int size) {
    return backtrace(buffer, size);
}

#endif
