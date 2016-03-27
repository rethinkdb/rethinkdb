// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef _WIN32

#include <unistd.h>

#include "logger.hpp"

#include "extproc/extproc_worker.hpp"
#include "extproc/extproc_spawner.hpp"
#include "arch/timing.hpp"
#include "arch/runtime/runtime.hpp"
#include "containers/archive/socket_stream.hpp"
#include "extproc/extproc_job.hpp"

// Guaranteed to be random, chosen by fair dice roll
const uint64_t extproc_worker_t::parent_to_worker_magic = 0x700168fe5380e17bLL;
const uint64_t extproc_worker_t::worker_to_parent_magic = 0x9aa9a30dc74f9da1LL;

NORETURN bool worker_exit_fn(read_stream_t *stream_in, write_stream_t *) {
    int exit_code;
    archive_result_t res = deserialize<cluster_version_t::LATEST_OVERALL>(
            stream_in, &exit_code);
    if (bad(res)) { exit_code = EXIT_FAILURE; }
    ::_exit(exit_code);
}

extproc_worker_t::extproc_worker_t(extproc_spawner_t *_spawner) :
    spawner(_spawner),
    worker_pid(-1),
    interruptor(nullptr) { }

extproc_worker_t::~extproc_worker_t() {
    if (worker_pid != -1) {
        socket_stream.create(socket.get(), reinterpret_cast<fd_watcher_t*>(NULL));

        try {
            run_job(&worker_exit_fn);

            int exit_code = 0;
            write_message_t wm;
            serialize<cluster_version_t::LATEST_OVERALL>(&wm, exit_code);
            int res = send_write_message(get_write_stream(), &wm);
            if (res != 0) {
                throw extproc_worker_exc_t("failed to send exit code to worker");
            }

            socket_stream.reset();

            // TODO: Give some time to exit, then kill
        } catch (const extproc_worker_exc_t &ex) {
            logERR("Failed to shutdown worker process: '%s'.  Killing it.", ex.what());
            socket_stream.reset();
            kill_process();
        }
    }
}

void extproc_worker_t::acquired(signal_t *_interruptor) {
    // Only start the worker process once the worker is acquired for the first time
    // This will also repair a killed worker

    // We create the streams here, since they are thread-dependant
    if (worker_pid == -1) {
        socket.reset(spawner->spawn(&socket_stream, &worker_pid));
    } else {
        socket_stream.create(socket.get(), reinterpret_cast<fd_watcher_t*>(NULL));
    }

    // Apply the user interruptor to our stream along with the extproc pool's interruptor
    guarantee(interruptor == nullptr);
    interruptor = _interruptor;
    guarantee(interruptor != nullptr);
    socket_stream.get()->set_interruptor(interruptor);
}

void extproc_worker_t::released(bool user_error, signal_t *user_interruptor) {
    guarantee(interruptor != nullptr);
    bool errored = user_error;

    // If we were interrupted by the user, we can't count on the worker being valid
    if (user_interruptor != nullptr && user_interruptor->is_pulsed()) {
        errored = true;
    } else if (!errored) {
        // Set up a timeout interruptor for the final write/read
        signal_timer_t timeout;
        wait_any_t final_interruptor(&timeout, interruptor);
        socket_stream->set_interruptor(&final_interruptor);
        timeout.start(100); // Allow 100ms for the child to respond

        // Trade magic numbers with worker process to see if it is still coherent
        try {
            write_message_t wm;
            serialize<cluster_version_t::LATEST_OVERALL>(&wm, parent_to_worker_magic);
            {
                int res = send_write_message(socket_stream.get(), &wm);
                if (res != 0) {
                    throw extproc_worker_exc_t("failed to send magic number");
                }
            }


            uint64_t magic_from_child;
            archive_result_t res
                = deserialize<cluster_version_t::LATEST_OVERALL>(socket_stream.get(),
                                                                 &magic_from_child);
            if (bad(res) || magic_from_child != worker_to_parent_magic) {
                throw extproc_worker_exc_t("did not receive magic number");
            }
        } catch (...) {
            // This would indicate a job is hanging or not reading out all its data
            if (!final_interruptor.is_pulsed() || timeout.is_pulsed()) {
                logERR("worker process failed to resynchronize with main process");
            }
            errored = true;
        }
    }

    socket_stream.reset();
    interruptor = nullptr;

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

bool extproc_worker_t::is_process_alive()
{
    return (worker_pid != -1);
}

void extproc_worker_t::run_job(bool (*fn) (read_stream_t *, write_stream_t *)) {
    write_message_t wm;
    wm.append(&fn, sizeof(fn));

    int res = send_write_message(get_write_stream(), &wm);
    if (res != 0) {
        throw extproc_worker_exc_t("failed to send job function to worker");
    }
}

read_stream_t *extproc_worker_t::get_read_stream() {
    return socket_stream.get();
}

write_stream_t *extproc_worker_t::get_write_stream() {
    return socket_stream.get();
}

#endif
