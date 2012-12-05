// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "extproc/job.hpp"
#include "clustering/administration/proc_stats.hpp"

namespace extproc {

bool is_spawner_alive(pid_t spawner_pid) {
    bool retval = false;
    try {
        proc_pid_stat_t stats = proc_pid_stat_t::for_pid(spawner_pid);
        switch (stats.state) {
            case 'R':
            case 'S':
            case 'D':
            case 'T':
            case 'W':
                retval = true;
                break;
            default:
                break;
        }
    } catch (const std::runtime_error &ex) {
        // Failed to check stats, assume the rdb process has died
    }

    return retval;
}

// ---------- job_t ----------
int job_t::accept_job(control_t *control, void *extra) {
    // Try to receive the job.
    job_t::func_t jobfunc;
    int64_t res = force_read(control, &jobfunc, sizeof(jobfunc));
    if (res < (int64_t) sizeof(jobfunc)) {
        // Don't log anything if the parent isn't alive, it likely means there was an unclean shutdown,
        //  and the file descriptor is invalid.  We don't want to pollute the output.
        if (is_spawner_alive(control->get_spawner_pid())) {
            control->log("Couldn't read job function: %s",
                          res == -1 ? errno_string(errno).c_str() : "end-of-file received");
        }
        return -1;
    }

    // Run the job.
    (*jobfunc)(control, extra);
    return 0;
}

void job_t::append_to(write_message_t *msg) const {
    // This is kind of a hack.
    //
    // We send the address of the function that runs the job we want. This works
    // only because the worker processes we are sending to are fork()s of
    // ourselves, and so have the same address space layout.
    job_t::func_t funcptr = job_runner();
    msg->append(&funcptr, sizeof(funcptr));

    // We send the job over as well; job_runner will deserialize it.
    this->rdb_serialize(*msg);
}

int job_t::send_over(write_stream_t *stream) const {
    write_message_t msg;
    append_to(&msg);
    return send_write_message(stream, &msg);
}


// ---------- job_t::control_t ----------
job_t::control_t::control_t(pid_t _pid, pid_t _spawner_pid, scoped_fd_t *fd) :
    unix_socket_stream_t(fd, new blocking_fd_watcher_t()),
    pid(_pid),
    spawner_pid(_spawner_pid)
{ }

pid_t job_t::control_t::get_spawner_pid() const {
    return spawner_pid;
}

// TODO(rntz): some way to send log messages to the engine.
void job_t::control_t::vlog(const char *fmt, va_list ap) {
    flockfile(stderr);
    fprintf(stderr, "[%d] worker: ", pid);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    funlockfile(stderr);
}

void job_t::control_t::log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(fmt, ap);
    va_end(ap);
}



} // namespace extproc
