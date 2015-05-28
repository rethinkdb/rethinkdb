// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_BLOCKER_POOL_HPP_
#define ARCH_IO_BLOCKER_POOL_HPP_

#include <pthread.h>

#include <vector>

#include "arch/runtime/event_queue.hpp"
#include "arch/io/concurrency.hpp"
#include "arch/runtime/system_event.hpp"

class blocker_pool_t : public event_callback_t {
public:
    blocker_pool_t(int nthreads, event_queue_t *queue);
    ~blocker_pool_t();

    struct job_t {
        /* run() will not be run within the main thread pool. It may call blocking system calls and
        the like without disrupting performance of the main server thread pool. */
        virtual void run() = 0;

        /* done() will be called within the main thread pool once run() is done. */
        virtual void done() = 0;

    protected:
        virtual ~job_t() {}
    };
    void do_job(job_t *job);

private:
    static void *event_loop(void*);

    std::vector<pthread_t> threads;
    bool shutting_down;
    std::vector<job_t*> outstanding_requests;
    system_mutex_t or_mutex;
    system_cond_t or_cond;

    std::vector<job_t*> completed_events;
    event_queue_t *queue;
    system_event_t ce_signal;
    system_mutex_t ce_mutex;
    void on_event(int);
};

bool i_am_in_blocker_pool_thread();

#endif /* ARCH_IO_BLOCKER_POOL_HPP_ */
