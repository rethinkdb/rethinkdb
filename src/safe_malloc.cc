// Copyright 2010-2014 RethinkDB, all rights reserved.

#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h> // TODO!

#include "errors.hpp"

/* This file adds wrappers around malloc, calloc, realloc, valloc, memalign and
posix_memalign.
Those wrappers are there to make sure that allocation failures are not ignored.
Instead the wrappers crash through `crash_oom()`.

While it would be easy to replace malloc in our own code, this trick makes
sure that we don't forget checking the return values of library functions that
internally depend on memory allocation. It also makes it safer to include
third-party code into our codebase. */

// A template for allocation functions that return a void *, and NULL on error
// We must specialize this on the fun_name, because its static variables must be
// different ones for different wrapped functions.
template<const char *fun_name, class... fun_arg_types>
void *safe_alloc_wrapper(fun_arg_types... args) {
    static void *(*other_fun)(fun_arg_types...) = NULL;
    // We need this `first_run` hack because we call `dlsym()`, and `dlsym()` itself
    // may call allocation functions again (specifically calloc). We detect that case
    // and return NULL/ENOMEM if that happens.
    static bool first_run = true;

    fprintf(stderr, "Entering %s\n", fun_name);
    if (first_run) {
        fprintf(stderr, "First run %s\n", fun_name);
        first_run = false;
        other_fun = (void *(*)(fun_arg_types...))dlsym(RTLD_NEXT, fun_name);
    } else if (other_fun == NULL) {
        fprintf(stderr, "Recursive call %s\n", fun_name);
        // We have been called by the call to `dlsym()` that we initiated above.
        // Return NULL and hope that it can live with that.
        errno = ENOMEM;
        return NULL;
    }
    fprintf(stderr, "Inner call %s\n", fun_name);
    void *ret = other_fun(args...);
    fprintf(stderr, "Done %s: %p\n", fun_name, ret);
    if (ret == NULL) {
        fprintf(stderr, "Crashing %s\n", fun_name);
        crash_oom(); // TODO! Replace this by something that has a slightly better error message.
    }
    return ret;
}

extern "C" {

const char MALLOC_FUN_NAME[] = "malloc";
void *malloc(size_t n) {
    return safe_alloc_wrapper<MALLOC_FUN_NAME, size_t>(n);
}

const char CALLOC_FUN_NAME[] = "calloc";
void *calloc(size_t n, size_t c) {
    return safe_alloc_wrapper<CALLOC_FUN_NAME, size_t, size_t>(n, c);
}

const char VALLOC_FUN_NAME[] = "valloc";
void *valloc(size_t n) {
    return safe_alloc_wrapper<VALLOC_FUN_NAME, size_t>(n);
}

const char MEMALIGN_FUN_NAME[] = "memalign";
void *memalign(size_t a, size_t n) {
    return safe_alloc_wrapper<MEMALIGN_FUN_NAME, size_t, size_t>(a, n);
}

const char REALLOC_FUN_NAME[] = "realloc";
void *realloc(void *ptr, size_t n) {
    return safe_alloc_wrapper<REALLOC_FUN_NAME, void *, size_t>(ptr, n);
}

int posix_memalign(void **ptr, size_t a, size_t n) {
    static int(*other_fun)(void **, size_t, size_t) = NULL;
    static bool first_run = true;

    if (first_run) {
        first_run = false;
        other_fun = (int(*)(void **, size_t, size_t))dlsym(RTLD_NEXT, "posix_memalign");
    } else {
        if (other_fun == NULL) {
            // We have been called by the call to `dlsym()` that we initiated above.
            // Return NULL and hope that it can live with that.
            return ENOMEM;
        }
    }
    int ret = other_fun(ptr, a, n);
    if (ret != 0) {
        crash_oom();
    }
    return 0;
}

} // extern "C"

// This function must be called once before switching into a multi-threaded context.
void init_safe_malloc() {
    const size_t test_size = sizeof(void *);
    // We call each function once to initialize their state while we are still
    // in a single thread.
    fprintf(stderr, "Init\n");
    void *test = malloc(test_size);
    fprintf(stderr, "Got test ptr: %p\n", test);
    free(test);
    free(realloc(malloc(test_size), test_size));
    fprintf(stderr, ".");
    free(calloc(1, test_size));
    fprintf(stderr, ".");
    free(valloc(test_size));
    fprintf(stderr, ".");
    free(memalign(test_size, test_size));
    fprintf(stderr, ".");
    void *ptr = NULL;
    posix_memalign(&ptr, test_size, test_size);
    free(ptr);
    fprintf(stderr, "Survived init\n");
}