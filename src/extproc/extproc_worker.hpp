// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef EXTPROC_EXTPROC_WORKER_HPP_
#define EXTPROC_EXTPROC_WORKER_HPP_

#include <sys/types.h>
#include "arch/process.hpp"
#include "arch/io/io_utils.hpp"
#include "concurrency/wait_any.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "containers/object_buffer.hpp"
#include "containers/scoped.hpp"
#include "containers/archive/socket_stream.hpp"
#include "containers/archive/archive.hpp"

class extproc_spawner_t;

class extproc_worker_t {
public:
    explicit extproc_worker_t(extproc_spawner_t *_spawner);
    ~extproc_worker_t();

    // Called whenever the worker changes hands (system -> user -> system)
    void acquired(signal_t *_interruptor);
    void released(bool user_error, signal_t *user_interruptor);

    // We accept jobs as functions that take a read stream and write stream
    //  so that they can communicate back to the job in the main process
    void run_job(bool (*fn) (read_stream_t *, write_stream_t *));

    read_stream_t *get_read_stream();
    write_stream_t *get_write_stream();

    void kill_process();
    bool is_process_alive();

    static const uint64_t parent_to_worker_magic;
    static const uint64_t worker_to_parent_magic;

private:
    void spawn();

    // This will run inside the blocker pool so the worker process doesn't inherit any
    //  of our coroutine stuff
    void spawn_internal();

    extproc_spawner_t *spawner;
    process_id_t worker_pid;
    scoped_fd_t socket;

    object_buffer_t<socket_stream_t> socket_stream;

#ifdef _WIN32
    object_buffer_t<windows_event_watcher_t> socket_event_watcher;
#endif

    signal_t *interruptor;
};

#endif /* EXTPROC_EXTPROC_WORKER_HPP_ */
