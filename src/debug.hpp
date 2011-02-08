#ifndef __DEBUG_HPP__
#define __DEBUG_HPP__

#ifdef __linux__
#if defined __i386 || defined __x86_64
#define BREAKPOINT __asm__ volatile ("int3")
#else   /* not x86/amd64 */
#define BREAKPOINT raise(SIGTRAP)
#endif  /* x86/amd64 */
 #endif /* __linux__ */


#ifndef NDEBUG
#define DEBUG_ONLY(expr) do { expr; } while (0);
#else
#define DEBUG_ONLY(expr) ((void)(0))
#endif

#endif // __DEBUG_HPP__
