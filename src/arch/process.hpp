#ifndef ARCH_PROCESS_HPP_
#define ARCH_PROCESS_HPP_

#ifdef _WIN32

#include "windows.hpp"

typedef DWORD process_id_t;

const process_id_t INVALID_PROCESS_ID = 0;

inline process_id_t current_process() {
    return GetCurrentProcessId();
}

#else

#include "rpc/serialize_macros.hpp"

typedef pid_t process_id_t;

const process_id_t INVALID_PROCESS_ID = -1;

inline process_id_t current_process() {
    return getpid();
}

#endif

#endif
