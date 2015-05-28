#ifndef ARCH_COMPILER_HPP_
#define ARCH_COMPILER_HPP_

// ATN: TODO

#ifdef _MSC_VER

#define ATTR_ALIGNED(size) __declspec(align(size))
#define ATTR_PACKED(decl) __pragma(pack(push, 1)) decl __pragma(pack(pop))
#define ATTR_FORMAT(x)
#define ATTR_NORETURN __declspec(noreturn)
#define DECL_THREAD_LOCAL thread_local // ATN __declspec(thread)
#define NOINLINE __declspec(noinline)
#define CURRENT_FUNCTION_PRETTY __FUNCSIG__

#else

#define ATTR_ALIGNED(size) __attribute__((aligned(size)))
#define ATTR_PACKED(decl) decl __attribute__((__packed__))
#define ATTR_FORMAT(x) __attribute__((format(x)))
#define ATTR_NORETURN __attribute__((noreturn))
#define DECL_THREAD_LOCAL __thread;
#define NOINLINE __attribute__ ((noinline))
#defien CURRENT_FUCTION_PRETTY __PRETTY_FUNCTION__

#endif

#endif