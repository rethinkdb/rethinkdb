#ifndef ARCH_PROCESS_HPP_
#define ARCH_PROCESS_HPP_

#if defined(_WIN32) && !defined(__MINGW32__)
typedef DWORD pid_t;
#endif

#endif  // ARCH_PROCESS_HPP_
