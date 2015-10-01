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

#include "rpc/serialize_macros.hpp"

class process_ref_t {
    // TODO ATN
public:
    process_ref_t(pid_t pid_) : pid(pid_) { }
    operator pid_t () { return pid; }
    static const pid_t invalid = -1;
    RDB_MAKE_ME_SERIALIZABLE_1(process_ref_t, pid);

private:
    pid_t pid;
};

inline process_ref_t current_process() {
    return getpid();
}

#endif

#endif
