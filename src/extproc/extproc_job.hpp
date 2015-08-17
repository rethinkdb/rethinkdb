// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef EXTPROC_EXTPROC_JOB_HPP_
#define EXTPROC_EXTPROC_JOB_HPP_

#include <exception>
#include <string>

#include "utils.hpp"
#include "containers/archive/archive.hpp"
#include "concurrency/wait_any.hpp"
#include "concurrency/cross_thread_semaphore.hpp"
#include "containers/object_buffer.hpp"

class extproc_pool_t;
class extproc_worker_t;

// Error that may be thrown when something goes wrong with a worker
// Should be used by any derived extproc tasks (like HTTP or JS)
class extproc_worker_exc_t : public std::exception {
public:
    explicit extproc_worker_exc_t(const std::string& data) : info(data) { }
    ~extproc_worker_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

class extproc_job_t : public home_thread_mixin_t {
public:
    extproc_job_t(extproc_pool_t *_pool,
                  bool (*worker_fn) (read_stream_t *, write_stream_t *),
                  signal_t *_user_interruptor);
    ~extproc_job_t();

    // All data written and read by the user must be accounted for, or the worker will
    //  has to be killed and restarted
    read_stream_t *read_stream();
    write_stream_t *write_stream();

    // Called by a user when an error occurs, and they want to mark the worker as 'dirty'
    //  so that it will be killed later.  This will skip the step where the main process
    //  attempts to resynchronize with the worker.
    void worker_error();

private:
    extproc_pool_t *pool;
    bool user_error;
    signal_t *user_interruptor;
    wait_any_t combined_interruptor;
    object_buffer_t<cross_thread_semaphore_t<extproc_worker_t>::lock_t> worker_lock;
};

#endif /* EXTPROC_EXTPROC_JOB_HPP_ */
