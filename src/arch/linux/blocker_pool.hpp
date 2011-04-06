#ifndef __ARCH_LINUX_BLOCKER_POOL_HPP__
#define __ARCH_LINUX_BLOCKER_POOL_HPP__

#include "arch/linux/event_queue.hpp"
#include "arch/linux/concurrency.hpp"
#include "arch/linux/system_event.hpp"
#include <pthread.h>

struct blocker_pool_t : public linux_event_callback_t {

    blocker_pool_t(int nthreads, linux_event_queue_t *queue);
    ~blocker_pool_t();

    struct job_t {
        /* run() will not be run within the main thread pool. It may call blocking system calls and
        the like without disrupting performance of the main server thread pool. */
        virtual void run() = 0;

        /* done() will be called within the main thread pool once run() is done. */
        virtual void done() = 0;

        virtual ~job_t() {}
    };
    void do_job(job_t *job);

private:
    std::vector<pthread_t> threads;
    static void *event_loop(void*);

    bool shutting_down;
    std::vector<job_t*> outstanding_requests;
    system_mutex_t or_mutex;
    system_cond_t or_cond;

    std::vector<job_t*> completed_events;
    linux_event_queue_t *queue;
    system_event_t ce_signal;
    system_mutex_t ce_mutex;
    void on_event(int);
};

#endif /* __ARCH_LINUX_BLOCKER_POOL_HPP__ */
