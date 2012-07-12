#include "extproc/job.hpp"

namespace extproc {

// ---------- job_t ----------
int job_t::accept_job(control_t *control) {
    // Try to receive the job.
    void (*jobfunc)(control_t*);
    int64_t res = force_read(control, &jobfunc, sizeof jobfunc);
    if (res < (int64_t) sizeof jobfunc) {
        control->log("Couldn't read job function: %s",
                      res == -1 ? strerror(errno) : "end-of-file received");
        return -1;
    }

    // Run the job.
    (*jobfunc)(control);
    return 0;
}

int job_t::send(write_stream_t *stream) {
    write_message_t msg;

    // This is kind of a hack.
    //
    // We send the address of the function that runs the job we want. This works
    // only because the worker processes we are sending to are fork()s of
    // ourselves, and so have the same address space layout.
    job_t::func_t funcptr = job_runner();
    msg.append(&funcptr, sizeof funcptr);

    // We send the job over as well; job_runner will deserialize it.
    this->rdb_serialize(msg);

    return send_write_message(stream, &msg);
}


// ---------- job_t::control_t ----------
job_t::control_t::control_t(int pid, int fd) :
    unix_socket_stream_t(fd, new blocking_fd_watcher_t()),
    pid_(pid)
{
}

// TODO(rntz): some way to send log messages to the engine.
void job_t::control_t::vlog(const char *fmt, va_list ap) {
    std::string real_fmt = strprintf("[%d] worker: %s\n", pid_, fmt);
    vfprintf(stderr, real_fmt.c_str(), ap);
}

void job_t::control_t::log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(fmt, ap);
    va_end(ap);
}



} // namespace extproc
