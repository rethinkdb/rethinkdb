/*
*/

#ifndef CORO_DEFINED
#define CORO_DEFINED 1

#ifdef VALGRIND
#define USE_VALGRIND
#endif

#include "taskimpl.hpp"


/*#if defined(__SYMBIAN32__)
	#define CORO_STACK_SIZE     8192
	#define CORO_STACK_SIZE_MIN 1024
#else
	//#define CORO_DEFAULT_STACK_SIZE     (65536/2)
	//#define CORO_DEFAULT_STACK_SIZE  (65536*4)

	//128k needed on PPC due to parser
	#define CORO_DEFAULT_STACK_SIZE (128*1024)
	#define CORO_STACK_SIZE_MIN 8192
#endif*/
//TODO: In a special testing mode, munmap the page above (below)
//the stack so we can get an error if it overflows
#define CORO_DEFAULT_STACK_SIZE 65536
#define CORO_STACK_SIZE_MIN 1024

#if !defined(__MINGW32__) && defined(WIN32)
#if defined(BUILDING_CORO_DLL) || defined(BUILDING_IOVMALL_DLL)
#define CORO_API __declspec(dllexport)
#else
#define CORO_API __declspec(dllimport)
#endif

#else
#define CORO_API
#endif

/*
#if defined(__amd64__) && !defined(__x86_64__)
	#define __x86_64__ 1
#endif
*/

// Pick which coro implementation to use
// The make file can set -DUSE_FIBERS, -DUSE_UCONTEXT or -DUSE_SETJMP to force this choice.
#if !defined(USE_FIBERS) && !defined(USE_UCONTEXT) && !defined(USE_SETJMP)

#if defined(WIN32) && defined(HAS_FIBERS)
#	define USE_FIBERS
#elif defined(HAS_UCONTEXT)
//#elif defined(HAS_UCONTEXT) && !defined(__x86_64__)
#	if !defined(USE_UCONTEXT)
#		define USE_UCONTEXT
#	endif
#else
#	define USE_SETJMP
#endif

#endif

#if defined(USE_FIBERS)
	#define CORO_IMPLEMENTATION "fibers"
#elif defined(USE_UCONTEXT)
	#include <sys/ucontext.h>
	#define CORO_IMPLEMENTATION "ucontext"
#elif defined(USE_SETJMP)
	#include <setjmp.h>
	#define CORO_IMPLEMENTATION "setjmp"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Coro Coro;

struct Coro
{
	size_t requestedStackSize;
	size_t allocatedStackSize;
	void *stack;

#ifdef USE_VALGRIND
	unsigned int valgrindStackId;
#endif

#if defined(USE_FIBERS)
	void *fiber;
#elif defined(USE_UCONTEXT)
	ucontext_t env;
#elif defined(USE_SETJMP)
	jmp_buf env;
#endif

	unsigned char isMain;
};

CORO_API Coro *Coro_new(void);
CORO_API void Coro_free(Coro *self);

// stack

CORO_API void *Coro_stack(Coro *self);
CORO_API size_t Coro_stackSize(Coro *self);
CORO_API void Coro_setStackSize_(Coro *self, size_t sizeInBytes);
CORO_API size_t Coro_bytesLeftOnStack(Coro *self);
CORO_API int Coro_stackSpaceAlmostGone(Coro *self);

CORO_API void Coro_initializeMainCoro(Coro *self);

typedef void (CoroStartCallback)(void *);

CORO_API void Coro_startCoro_(Coro *self, Coro *other, void *context, CoroStartCallback *callback);
CORO_API void Coro_switchTo_(Coro *self, Coro *next);
CORO_API void Coro_setup(Coro *self, void *arg); // private

#ifdef __cplusplus
}
#endif
#endif
