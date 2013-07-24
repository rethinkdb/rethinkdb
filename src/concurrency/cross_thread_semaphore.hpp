// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_CROSS_THREAD_SEMAPHORE_HPP_
#define CONCURRENCY_CROSS_THREAD_SEMAPHORE_HPP_

#include "arch/io/io_utils.hpp"
#include "arch/io/event_watcher.hpp"
#include "concurrency/one_per_thread.hpp"

// This class implements a semaphore that can be used across threads

// The linux version uses eventfds so we can use the semaphore without switching threads on every
//  operation, which kills performance when the server is under load
class cross_thread_semaphore_t {
public:
    explicit cross_thread_semaphore_t(size_t initial_value);

    void lock(signal_t *interruptor);
    void unlock();

private:
    // This class maintains a list of coroutines that are waiting for the eventfd on this thread
    class blocker_t {
    public:
        explicit blocker_t(cross_thread_semaphore_t *_parent);

        // Wait until we get a read event, then try to read again until success
        void lock_blocking(signal_t *interruptor);
        bool waiting();

    private:
        class lock_request_t : public signal_t, public intrusive_list_node_t<lock_request_t> {
        public:
            explicit lock_request_t(intrusive_list_t<lock_request_t> *_list);
            ~lock_request_t();

        private:
            void notify();

            intrusive_list_t<lock_request_t> *list;
        };

        class error_callback_t : public linux_event_callback_t {
            void on_event(UNUSED int events) {
                guarantee(false, "error detected on eventfd semaphore");
            }
        } error_callback;

        cross_thread_semaphore_t *parent;
        linux_event_watcher_t event_watcher;
        intrusive_list_t<lock_request_t> waiters;
    };

    bool try_lock();

    scoped_fd_t sem;
#if !defined(__linux) || defined(NO_EVENTFD)
    // Without eventfds, we implement this using a pipe, so we need another descriptor
    scoped_fd_t write_fd;
#endif

    object_buffer_t<one_per_thread_t<blocker_t> > thread_blockers;
};

#endif /* CONCURRENCY_CROSS_THREAD_SEMAPHORE_HPP_ */
