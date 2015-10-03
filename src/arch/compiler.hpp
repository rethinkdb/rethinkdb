#ifndef ARCH_COMPILER_HPP_
#define ARCH_COMPILER_HPP_

// ATN: TODO

#ifdef _MSC_VER // msc

#define ATTR_ALIGNED(size) __declspec(align(size))
#define ATTR_PACKED(...) __pragma(pack(push, 1)) __VA_ARGS__ __pragma(pack(pop))
#define ATTR_FORMAT(...)
#define ATTR_NORETURN __declspec(noreturn)
#define DECL_THREAD_LOCAL thread_local // ATN TODO which is better, this or __declspec(thread)?
#define NOINLINE __declspec(noinline)
#define CURRENT_FUNCTION_PRETTY __FUNCSIG__

#else // gcc, clang

#define ATTR_ALIGNED(size) __attribute__((aligned(size)))
#define ATTR_PACKED(...) __VA_ARGS__ __attribute__((__packed__))

#ifdef __MINGW32__
#define ATTR_FORMAT(...) // mingw64 complains about PRIu64
#else
#define ATTR_FORMAT(...) __attribute__((format(__VA_ARGS__)))
#endif

#define ATTR_NORETURN __attribute__((noreturn))
#define DECL_THREAD_LOCAL __thread
#define NOINLINE __attribute__ ((noinline))
#define CURRENT_FUNCTION_PRETTY __PRETTY_FUNCTION__

#endif

#endif
