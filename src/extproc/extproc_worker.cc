// Copyright 2010-2013 RethinkDB, all rights reserved.
#include <unistd.h>

#include "logger.hpp"

#include "extproc/extproc_worker.hpp"
#include "extproc/extproc_spawner.hpp"
#include "arch/timing.hpp"
#include "arch/runtime/runtime.hpp"
#include "containers/archive/socket_stream.hpp"

// Guaranteed to be random, chosen by fair dice roll
const uint64_t extproc_worker_t::parent_to_worker_magic = 0x700168fe5380e17b;
const uint64_t extproc_worker_t::worker_to_parent_magic = 0x9aa9a30dc74f9da1;

NORETURN bool worker_exit_fn(read_stream_t *stream_in, write_stream_t *) {
    int exit_code;
    int res = deserialize(stream_in, &exit_code);
    if (res != ARCHIVE_SUCCESS) { exit_code = EXIT_FAILURE; }
    ::_exit(exit_code);
}

extproc_worker_t::extproc_worker_t(scoped_array_t<scoped_ptr_t<cross_thread_signal_t> > *_interruptors,
                                   extproc_spawner_t *_spawner) :
    spawner(_spawner),
    worker_pid(-1),
    interruptors(_interruptors),
    user_interruptor(NULL) { }

extproc_worker_t::~extproc_worker_t() {
    if (worker_pid != -1) {
        socket_stream.create(socket.get(), reinterpret_cast<fd_watcher_t*>(NULL));

        // TODO: check that worker is extant and/or catch exceptions
        run_job(&worker_exit_fn);

        int exit_code = 0;
        write_message_t msg;
        msg << exit_code;
        int res = send_write_message(get_write_stream(), &msg);

        socket_stream.reset();

        if (res != 0) {
            logERR("Could not shut down worker orderly, killing it...");
            kill_process();
        } else {
            // TODO: Give some time to exit, then kill
        }
    }
}

void extproc_worker_t::acquired(signal_t *_user_interruptor) {
    // Only start the worker process once the worker is acquired for the first time
    // This will also repair a killed worker

    // We create the streams here, since they are thread-dependant
    if (worker_pid == -1) {
        socket.reset(spawner->spawn());
        socket_stream.create(socket.get(), reinterpret_cast<fd_watcher_t*>(NULL));

        // Get the pid of the new worker process
        int res = deserialize(socket_stream.get(), &worker_pid);
        guarantee(res == ARCHIVE_SUCCESS);
        guarantee(worker_pid != -1);
    } else {
        socket_stream.create(socket.get(), reinterpret_cast<fd_watcher_t*>(NULL));
    }

    // Apply the user interruptor to our stream along with the extproc pool's interruptor
    user_interruptor = _user_interruptor;
    if (user_interruptor == NULL) {
        socket_stream.get()->set_interruptor((*interruptors)[get_thread_id()].get());
    } else {
        combined_interruptor.create((*interruptors)[get_thread_id()].get(), user_interruptor);
        socket_stream.get()->set_interruptor(combined_interruptor.get());
    }
}

void extproc_worker_t::released() {
    bool errored = false;

    // If we were interrupted by the user, we can't count on the worker being valid
    if (user_interruptor != NULL && user_interruptor->is_pulsed()) {
        errored = true;
    } else {
        // Trade magic numbers with worker process to see if it is still coherent
        try {
            write_message_t msg;
            msg << parent_to_worker_magic;
            int res = send_write_message(socket_stream.get(), &msg);
            if (res != 0) { throw std::runtime_error("failed to send magic number"); }

            // Set up a timeout interruptor for the final read
            // We don't include the extproc pool's interruptor here for good reason
            signal_timer_t timeout;
            wait_any_t final_interruptor(&timeout);
            if (user_interruptor != NULL) {
                final_interruptor.add(user_interruptor);
            }
            socket_stream->set_interruptor(&final_interruptor);

            timeout.start(100); // Allow 100ms for the child to respond

            uint64_t magic_from_child;
            res = deserialize(socket_stream.get(), &magic_from_child);
            if (res != ARCHIVE_SUCCESS || magic_from_child != worker_to_parent_magic) {
                throw std::runtime_error("did not receive magic number");
            }
        } catch (...) {
            // Only log this if there is a serious problem
            // This would indicate a job is not reading out all its data
            if (user_interruptor == NULL || !user_interruptor->is_pulsed()) {
                logERR("worker process failed to resynchronize with main process");
            }
            errored = true;
        }
    }

    socket_stream.reset();

    if (combined_interruptor.has()) {
        combined_interruptor.reset();
    }

    // If anything went wrong, we just kill the worker and recreate it later
    if (errored) {
        kill_process();
    }
}

void extproc_worker_t::kill_process() {
    guarantee(worker_pid != -1);

    ::kill(worker_pid, SIGKILL);
    worker_pid = -1;

    // Clean up our socket fd
    socket.reset();
}

void extproc_worker_t::run_job(bool (*fn) (read_stream_t *, write_stream_t *)) {
    write_message_t msg;
    msg.append(&fn, sizeof(fn));
    int res = send_write_message(get_write_stream(), &msg);
    guarantee(res == 0);
}

read_stream_t *extproc_worker_t::get_read_stream() {
    return socket_stream.get();
}

write_stream_t *extproc_worker_t::get_write_stream() {
    return socket_stream.get();
}
