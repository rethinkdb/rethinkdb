#ifndef ARCH_PROCESS_HPP_
#define ARCH_PROCESS_HPP_

// TODO ATN

#ifdef _WIN32

#include "windows.hpp"

class process_ref_t {
public:
	static constexpr HANDLE invalid = nullptr;

	process_ref_t() : handle(nullptr) { }
	explicit process_ref_t(HANDLE handle_) : handle(handle_) { }
	operator HANDLE() { return handle;  }

	// ATN TODO: make sure all users of process_id_t destroy it promptly
	~process_ref_t() {
		if (handle != nullptr) {
			CloseHandle(handle);
		}
	}

	process_ref_t(process_ref_t &&other) : handle(other.handle) {
		other.handle = nullptr;
	}

private:
	DISABLE_COPYING(process_ref_t);

	HANDLE handle;
};

inline process_ref_t current_process() {
	return process_ref_t(GetCurrentProcess());
}

#else

class process_ref_t {
	pid_t pid;
	// ATN TODO: make this no-copy to imitate the behaviour of a process HANDLE on windows
};

#endif

#endif
