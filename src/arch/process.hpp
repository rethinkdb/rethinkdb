#ifndef ARCH_PROCESS_HPP_
#define ARCH_PROCESS_HPP_

// TODO ATN

#ifdef _WIN32

#include "windows.hpp"

struct process_id_t {
	DWORD pid;
};

#else

struct process_id_t {
	pid_t pid;
};

#endif

#endif