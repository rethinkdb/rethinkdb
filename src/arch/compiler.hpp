#ifndef ARCH_COMPILER_HPP_
#define ARCH_COMPILER_HPP_

// Compiler-specific macros

#ifdef _MSC_VER // msc

#define ATTR_ALIGNED(size) __declspec(align(size))
#define ATTR_PACKED(...) __pragma(pack(push, 1)) __VA_ARGS__ __pragma(pack(pop))
#define ATTR_FORMAT(...)
#define THREAD_LOCAL thread_local
#define CURRENT_FUNCTION_PRETTY __FUNCSIG__
#define NORETURN __declspec(noreturn)
#define UNUSED __pragma(warning(suppress: 4100 4101))
#define MUST_USE _Check_return_
#define NOINLINE __declspec(noinline)

#else // gcc, clang

#define ATTR_ALIGNED(size) __attribute__((aligned(size)))
#define ATTR_PACKED(...) __VA_ARGS__ __attribute__((__packed__))

#ifdef __MINGW32__
#define ATTR_FORMAT(...) // mingw64 complains about PRIu64
#else
#define ATTR_FORMAT(...) __attribute__((format(__VA_ARGS__)))
#endif

#define THREAD_LOCAL __thread
#define CURRENT_FUNCTION_PRETTY __PRETTY_FUNCTION__
#define NORETURN __attribute__((__noreturn__))
#define UNUSED __attribute__((unused))
#define MUST_USE __attribute__((warn_unused_result))
#define NOINLINE __attribute__((noinline))

#endif

#endif
