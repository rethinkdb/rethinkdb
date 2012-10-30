// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_FD_SEND_RECV_HPP_
#define ARCH_FD_SEND_RECV_HPP_

#include "errors.hpp"
#include "arch/runtime/runtime_utils.hpp" // fd_t

// WARNING. Each call to these functions involves copying `sizeof(int) *
// num_fds` bytes. Don't do it in a tight loop if `num_fds` is large. If you
// need to do this, refactor this code.

// Sends open file descriptors. Must be matched by a call to recv_fds() on the
// other end, or weird shit could happen.
//
// Return -1 on error, 0 on success. Sets errno on failure.
MUST_USE int send_fds(int socket_fd, size_t num_fds, fd_t *fds);

enum fd_recv_result_t {
    FD_RECV_OK, FD_RECV_ERROR, FD_RECV_EOF
};

// Receives open file descriptors. Must only be called to match a call to
// write_fds() on the other end; otherwise undefined behavior could result!
//
// Sets errno if it returns FD_RECV_ERROR.
MUST_USE fd_recv_result_t recv_fds(int socket_fd, size_t num_fds, fd_t *fds);

#endif // ARCH_FD_SEND_RECV_HPP_
