
#include <string>
#include <algorithm>
#include <retest.hpp>
#include <unistd.h>
#include <errno.h>
#include "fsm.hpp"
#include "alloc/malloc.hpp"
#include "operations.hpp"

using namespace std;

// Typedef test fsm
struct mock_io_calls_t;
typedef fsm_state_t<mock_io_calls_t, malloc_alloc_t> test_fsm_t;

// Mock IO
struct mock_io_calls_t {
    mock_io_calls_t()
        : soft_recvlim(2), hard_recvlim(255), bytes_received(0),
          soft_sendlim(2), hard_sendlim(255), bytes_sent(0)
        {}
    
    ssize_t read(resource_t fd, void *buf, size_t count) {
        // If we're over hard limit, reset the counter and return EAGAIN
        if(bytes_received >= hard_recvlim) {
            bytes_received = 0;
            errno = EAGAIN;
            return -1;
        }
        // Otherwise, limit ourselves to soft limit
        size_t lim = min((int)count, soft_recvlim);

        // And make sure we don't over the hard limit
        if(bytes_received + lim > hard_recvlim) {
            lim = hard_recvlim - bytes_received;
        }

        // Now we can "read" the data
        size_t res = recvbuf.copy((char*)buf, lim);
        recvbuf.erase(0, res);
        bytes_received += res;
        return res;
    }

    ssize_t write(resource_t fd, const void *buf, size_t count) {
        // If we're over hard limit, reset the counter and return EAGAIN
        if(bytes_sent >= hard_sendlim) {
            bytes_sent = 0;
            errno = EAGAIN;
            return -1;
        }
        
        // Otherwise, limit ourselves to soft limit
        size_t lim = min((int)count, soft_sendlim);

        // And make sure we don't over the hard limit
        if(bytes_sent + lim > hard_sendlim) {
            lim = hard_sendlim - bytes_sent;
        }

        sendbuf.append((char*)buf, lim);
        bytes_sent += lim;
        return lim;
    }

    string recvbuf;
    int soft_recvlim, hard_recvlim, bytes_received;
    
    string sendbuf;
    int soft_sendlim, hard_sendlim, bytes_sent;
};

// Mock ops
class mock_operations_t : public operations_t {
public:
    mock_operations_t() {}
    virtual result_t process_command(event_t *event) {
        test_fsm_t *state = (test_fsm_t*)event->state;
        if(strncmp(state->buf, "hello", 5) == 0) {
            strcpy(state->buf, "okey");
            state->nbuf = 5;
            return command_success_response_ready;
        } else if(strncmp(state->buf, "hello", 1) == 0 ||
                  strncmp(state->buf, "hello", 2) == 0 ||
                  strncmp(state->buf, "hello", 3) == 0 ||
                  strncmp(state->buf, "hello", 4) == 0)
        {
            return incomplete_command;
        } else {
            return malformed_command;
        }
        return command_success_no_response;
    }

private:
    rethink_tree_t *btree;
};

// Setup event
void setup_event(event_t *event, test_fsm_t *fsm) {
    event->event_type = et_sock;
    event->op = eo_rdwr;
    event->state = (event_state_t*)fsm;
}

// Unit test header helper
#define DEFINE_FSM(alloc, ops, event, fsm)  \
    malloc_alloc_t alloc;                   \
    mock_operations_t ops;                  \
    event_t event;                          \
    test_fsm_t fsm(0, &(alloc), &(ops));    \
    setup_event(&(event), &(fsm));
    

// A basic test - read a complete, well formed command, and expect the
// fsm to write a complete response. Note that we test soft limit
// loops but not hard limits here (see the io mock implementation).
void test_fsm_basic() {
    DEFINE_FSM(alloc, ops, event, fsm);

    fsm.recvbuf += "hello";
    fsm.do_transition(&event);
    assert_eq(strcmp(fsm.sendbuf.c_str(), "okey"), 0);
    assert_eq(fsm.state, test_fsm_t::fsm_socket_connected);
}

// Read a complete, malformed command, and expect the fsm to write a
// complete error response. Note that we test soft limit loops but not
// hard limits here (see the io mock implementation).
void test_fsm_malformed() {
    DEFINE_FSM(alloc, ops, event, fsm);

    fsm.recvbuf += "malformed";
    fsm.do_transition(&event);
    assert_eq(strcmp(fsm.sendbuf.c_str(), "(ERROR) Unknown command\n"), 0);
    assert_eq(fsm.state, test_fsm_t::fsm_socket_connected);
}

// Go through the well formed, incomplete receive cycle. We test both
// soft and hard limits here (see io mock implementation).
void test_fsm_incomplete_recv() {
    DEFINE_FSM(alloc, ops, event, fsm);
    fsm.hard_recvlim = 2;

    // Make sure we're transitioned to an incomplete state
    fsm.recvbuf += "hello";
    fsm.do_transition(&event);
    assert_eq(strcmp(fsm.sendbuf.c_str(), ""), 0);
    assert_eq(fsm.state, test_fsm_t::fsm_socket_recv_incomplete);

    // One more incomplete chunk
    fsm.do_transition(&event);
    assert_eq(strcmp(fsm.sendbuf.c_str(), ""), 0);
    assert_eq(fsm.state, test_fsm_t::fsm_socket_recv_incomplete);

    // Complete the command
    fsm.do_transition(&event);
    assert_eq(strcmp(fsm.sendbuf.c_str(), "okey"), 0);
    assert_eq(fsm.state, test_fsm_t::fsm_socket_connected);
}

// Go through the well formed, incomplete send cycle. We test both
// soft and hard limits here (see io mock implementation).
void test_fsm_incomplete_send() {
    DEFINE_FSM(alloc, ops, event, fsm);
    fsm.hard_sendlim = 2;

    // Make sure we're transitioned to an incomplete state
    fsm.recvbuf += "hello";
    fsm.do_transition(&event);
    assert_eq(strcmp(fsm.sendbuf.c_str(), "ok"), 0);
    assert_eq(fsm.state, test_fsm_t::fsm_socket_send_incomplete);

    // One more incomplete
    fsm.do_transition(&event);
    assert_eq(strcmp(fsm.sendbuf.c_str(), "okey"), 0);
    assert_eq(fsm.state, test_fsm_t::fsm_socket_send_incomplete);

    // And now we're done (sends the last \0)
    fsm.do_transition(&event);
    assert_eq(strcmp(fsm.sendbuf.c_str(), "okey"), 0);
    assert_eq(fsm.state, test_fsm_t::fsm_socket_connected);
}

