#ifndef CORO_DEFINED
#define CORO_DEFINED 1

#define CORO_DEFAULT_STACK_SIZE 65536

#if defined(USE_FIBERS)
    #define CORO_IMPLEMENTATION "fibers"
#elif defined(USE_UCONTEXT)
    #include <sys/ucontext.h>
    #define CORO_IMPLEMENTATION "ucontext"
#else
    #error "Coroutines aren't defined for this platform"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Coro Coro;

struct Coro
{
    void *stack;
    size_t stack_size;

#ifdef VALGRIND
    unsigned int valgrindStackId;
#endif

#if defined(USE_FIBERS)
    void *fiber;
#elif defined(USE_UCONTEXT)
    ucontext_t env;
#endif

    unsigned char isMain;
};

Coro *Coro_new(size_t);
void Coro_free(Coro *self);

// stack

void Coro_initializeMainCoro(Coro *self);
void Coro_switchTo_(Coro *self, Coro *next);
void Coro_setup(Coro *self);

#ifdef __cplusplus
}
#endif
#endif
