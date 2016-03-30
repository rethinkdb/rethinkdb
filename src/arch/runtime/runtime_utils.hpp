// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_RUNTIME_UTILS_HPP_
#define ARCH_RUNTIME_RUNTIME_UTILS_HPP_

#include <signal.h>
#include <stdint.h>
#include <unistd.h>

#include "config/args.hpp"
#include "containers/intrusive_list.hpp"

#ifdef _WIN32

typedef HANDLE fd_t;
const fd_t INVALID_FD = INVALID_HANDLE_VALUE;
const fd_t STDOUT_FD = GetStdHandle(STD_OUTPUT_HANDLE);
const fd_t STDERR_FD = GetStdHandle(STD_ERROR_HANDLE);

inline SOCKET fd_to_socket(fd_t handle) {
    CT_ASSERT(sizeof(SOCKET) == sizeof(HANDLE));
    return reinterpret_cast<SOCKET>(handle);
}

inline fd_t socket_to_fd(SOCKET s) {
    return reinterpret_cast<fd_t>(s);
}

#else

typedef int fd_t;
const fd_t INVALID_FD = -1;
const fd_t STDOUT_FD = STDOUT_FILENO;
const fd_t STDERR_FD = STDERR_FILENO;

inline int fd_to_socket(fd_t fd) {
    return fd;
}

inline fd_t socket_to_fd(int fd) {
    return fd;
}

#endif


class linux_thread_message_t : public intrusive_list_node_t<linux_thread_message_t> {
public:
    explicit linux_thread_message_t(int _priority)
        : priority(_priority),
        is_ordered(false)
#ifndef NDEBUG
        , reloop_count_(0)
#endif
        { }
    linux_thread_message_t()
        : priority(MESSAGE_SCHEDULER_DEFAULT_PRIORITY),
        is_ordered(false)
#ifndef NDEBUG
        , reloop_count_(0)
#endif
        { }
    virtual void on_thread_switch() = 0;

    void set_priority(int _priority) { priority = _priority; }
    int get_priority() const { return priority; }
protected:
    virtual ~linux_thread_message_t() {}
private:
    friend class linux_message_hub_t;
    int priority;
    bool is_ordered; // Used internally by the message hub
#ifndef NDEBUG
    int reloop_count_;
#endif
};

typedef linux_thread_message_t thread_message_t;

int get_cpu_count();

// More pollution of runtime_utils.hpp.
#ifndef NDEBUG

/* If `ASSERT_NO_CORO_WAITING;` appears at the top of a block, then it is illegal
to call `coro_t::wait()`, `coro_t::spawn_now_dangerously()`, or `coro_t::notify_now()`
within that block and any attempt to do so will be a fatal error. */
#define ASSERT_NO_CORO_WAITING \
    assert_no_coro_waiting_t assert_no_coro_waiting_var(__FILE__, __LINE__)

/* If `ASSERT_FINITE_CORO_WAITING;` appears at the top of a block, then code
within that block may call `coro_t::spawn_now_dangerously()` or `coro_t::notify_now()` but
not `coro_t::wait()`. This is because `coro_t::spawn_now_dangerously()` and
`coro_t::notify_now()` will return control directly to the coroutine that called
them. */
#define ASSERT_FINITE_CORO_WAITING assert_finite_coro_waiting_t assert_finite_coro_waiting_var(__FILE__, __LINE__)

/* Implementation support for `ASSERT_NO_CORO_WAITING` and `ASSERT_FINITE_CORO_WAITING` */
struct assert_no_coro_waiting_t {
    assert_no_coro_waiting_t(const char *, int);
    ~assert_no_coro_waiting_t();
};
struct assert_finite_coro_waiting_t {
    assert_finite_coro_waiting_t(const char *, int);
    ~assert_finite_coro_waiting_t();
};

#else  // NDEBUG

/* In release mode, these assertions are no-ops. */
#define ASSERT_NO_CORO_WAITING do { } while (0)
#define ASSERT_FINITE_CORO_WAITING do { } while (0)

#endif  // NDEBUG

#ifndef _WIN32
struct sigaction make_sa_handler(int sa_flags, void (*sa_handler_func)(int));
struct sigaction make_sa_sigaction(int sa_flags, void (*sa_sigaction_func)(int, siginfo_t *, void *));
#endif


#endif // ARCH_RUNTIME_RUNTIME_UTILS_HPP_
