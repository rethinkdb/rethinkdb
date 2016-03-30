// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef _WIN32

#include <sys/types.h>
#include <sys/socket.h>

#include <string.h>

#include "containers/scoped.hpp"
#include "arch/fd_send_recv.hpp"

// The code for {send,recv}_fds was determined by careful reading of the man
// pages for sendmsg(2), recvmsg(2), unix(7), and particularly cmsg(3), which
// contains a helpful example.
// <http://swtch.com/usr/local/plan9/src/lib9/sendfd.c> was also helpful.

int send_fds(int socket_fd, size_t num_fds, int *fds) {
    // Set-up for a call to sendmsg().
    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));

    // We send a single null byte along with the data. Sending a zero-length
    // message is dubious; receiving one is even more dubious.
    struct iovec iov;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    char c = '\0';
    iov.iov_base = &c;
    iov.iov_len = 1;

    // The control message is the important part for sending fds.
    scoped_array_t<char> control(CMSG_SPACE(sizeof(int) * num_fds));
    msg.msg_control = control.data();
    msg.msg_controllen = control.size();

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET; // see unix(7) for explanation
    cmsg->cmsg_type = SCM_RIGHTS;  // says that we're sending an array of fds
    cmsg->cmsg_len = CMSG_LEN(sizeof(int) * num_fds);
    memcpy(CMSG_DATA(cmsg), fds, sizeof(int) * num_fds);

    msg.msg_controllen = cmsg->cmsg_len;

    // Write it out.
    ssize_t res = sendmsg(socket_fd, &msg, 0);
    if (res == 1)
        return 0;               // success!

    guarantee(res == -1);
    return -1;
}

fd_recv_result_t recv_fds(int socket_fd, size_t num_fds, int *fds) {
    // Set-up for a call to recvmsg()
    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));

    // We expect to receive a single null byte.
    struct iovec iov;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    char c = 'x';               // a value that is not what we expect
    iov.iov_base = &c;
    iov.iov_len = 1;

    // The control message.
    scoped_array_t<char> control(CMSG_SPACE(sizeof(int) * num_fds));
    msg.msg_control = control.data();
    msg.msg_controllen = control.size();

    // Read it in.
    ssize_t res = recvmsg(socket_fd, &msg, 0);
    if (res == 0) return FD_RECV_EOF;
    if (res == -1) return FD_RECV_ERROR;

    guarantee(res == 1);

    // Check the message.
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    rassert(c == '\0' && cmsg &&
            msg.msg_controllen == CMSG_SPACE(sizeof(int) * num_fds) &&
            cmsg->cmsg_len == CMSG_LEN(sizeof(int) * num_fds));

    // Success!
    memcpy(fds, CMSG_DATA(cmsg), sizeof(int) * num_fds);
    return FD_RECV_OK;
}

#endif
