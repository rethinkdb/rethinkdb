// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef _WIN32 // TODO ATN
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/socket.h>
#endif
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "arch/process.hpp"
#include "extproc/extproc_spawner.hpp"
#include "extproc/extproc_worker.hpp"
#include "arch/fd_send_recv.hpp"

extproc_spawner_t *extproc_spawner_t::instance = NULL;

// This is the class that runs in the external process, doing all the work
class worker_run_t {
public:
    worker_run_t(fd_t _socket, pid_t _spawner_pid) :
        socket(_socket), socket_stream(socket.get(), &blocking_watcher) {
        guarantee(spawner_pid == -1);
        spawner_pid = _spawner_pid;

#ifndef _WIN32 // ATN TODO
        // Set ourselves to get interrupted when our parent dies
        struct sigaction sa = make_sa_handler(0, check_ppid_for_death);
        const int sigaction_res = sigaction(SIGALRM, &sa, NULL);
        guarantee_err(sigaction_res == 0, "worker: could not set action for ALRM signal");

        struct itimerval timerval;
        timerval.it_interval.tv_sec = 0;
        timerval.it_interval.tv_usec = 500 * THOUSAND;
        timerval.it_value = timerval.it_interval;
        struct itimerval old_timerval;
        const int itimer_res = setitimer(ITIMER_REAL, &timerval, &old_timerval);
        guarantee_err(itimer_res == 0, "worker: setitimer failed");
        guarantee(old_timerval.it_value.tv_sec == 0 && old_timerval.it_value.tv_usec == 0,
                  "worker: setitimer saw that we already had an itimer!");
#endif
        // Send our pid over to the main process (because it didn't fork us directly)
        write_message_t wm;
        serialize<cluster_version_t::LATEST_OVERALL>(&wm, getpid());
        int res = send_write_message(&socket_stream, &wm);
        guarantee(res == 0);
    }

    ~worker_run_t() {
        guarantee(spawner_pid != -1);
        spawner_pid = -1;
    }

    // Returning from this indicates an error, orderly shutdown will exit() manually
    void main_loop() {
        // Receive and run a function from the main process until one returns false
        bool (*fn) (read_stream_t *, write_stream_t *);
        while (true) {
            int64_t read_size = sizeof(fn);
            const int64_t read_res = force_read(&socket_stream, &fn, read_size);
            if (read_res != read_size) {
                break;
            }

            if (!fn(&socket_stream, &socket_stream)) {
                break;
            }

            // Trade magic numbers with the parent
            uint64_t magic_from_parent;
            {
                archive_result_t res =
                    deserialize<cluster_version_t::LATEST_OVERALL>(&socket_stream,
                                                                   &magic_from_parent);
                if (res != archive_result_t::SUCCESS ||
                    magic_from_parent != extproc_worker_t::parent_to_worker_magic) {
                    break;
                }
            }

            write_message_t wm;
            serialize<cluster_version_t::LATEST_OVERALL>(
                    &wm, extproc_worker_t::worker_to_parent_magic);
            int res = send_write_message(&socket_stream, &wm);
            if (res != 0) {
                break;
            }
        }
    }

private:
    static pid_t spawner_pid;

#ifndef _WIN32 // TODO ATN
    static void check_ppid_for_death(int) {
        pid_t ppid = getppid();
        if (spawner_pid != -1 && spawner_pid != ppid) {
            ::_exit(EXIT_FAILURE);
        }
    }
#endif

    scoped_fd_t socket;
    blocking_fd_watcher_t blocking_watcher;
    socket_stream_t socket_stream;
};

pid_t worker_run_t::spawner_pid = -1;

class spawner_run_t {
public:
    explicit spawner_run_t(fd_t _socket) :
        socket(_socket) {
#ifndef _WIN32 // ATN TODO
        // We set our PGID to our own PID (rather than inheriting our parent's PGID)
        // so that a signal (eg. SIGINT) sent to the parent's PGID (by eg. hitting
        // Ctrl-C at a terminal) will not propagate to us or our children.
        //
        // This is desirable because the RethinkDB engine deliberately crashes with
        // an error message if the spawner or a worker process dies; but a
        // command-line SIGINT should trigger a clean shutdown, not a crash.
        const int setpgid_res = setpgid(0, 0);
        guarantee_err(setpgid_res == 0, "spawner could not set PGID");

        // We ignore SIGCHLD so that we don't accumulate zombie children.
        {
            // NB. According to `man 2 sigaction` on linux, POSIX.1-2001 says that
            // this will prevent zombies, but may not be "fully portable".
            struct sigaction sa = make_sa_handler(0, SIG_IGN);
            const int res = sigaction(SIGCHLD, &sa, NULL);
            guarantee_err(res == 0, "spawner: Could not ignore SIGCHLD");
        }
#endif
    }

    void main_loop() {
        process_ref_t spawner_pid = current_process();

        while(true) {
#ifdef _WIN32
            rassert("TODO ATN: get worker_socket from socket? also test this.");
            char path[MAX_PATH];
            DWORD res = GetModuleFileName(NULL, path, sizeof(path));
            guarantee_winerr(res != 0 && res != sizeof(path), "GetModuleFileName failed");
            rassert("TODO ATN: rethinkdb start-worker");
            std::string command_line = strprintf("rethinkdb start-worker " /* ATN TODO */);
            std::vector<char> mutable_command_line(command_line.begin(), command_line.end());
            mutable_command_line.push_back('\0');
            PROCESS_INFORMATION process_info;
            CreateProcess(path, &mutable_command_line[0], nullptr, nullptr, true, 0,
                          nullptr, nullptr, nullptr, &process_info);
            CloseHandle(process_info.hProcess);
            CloseHandle(process_info.hThread);
#else
            fd_t worker_socket;
            fd_recv_result_t recv_res = recv_fds(socket.get(), 1, &worker_socket);
            if (recv_res != FD_RECV_OK) {
                break;
            }
            pid_t res = ::fork();
            if (res == 0) {
                // Worker process here
                socket.reset(); // Don't need the spawner's pipe
                worker_run_t worker_runner(worker_socket, spawner_pid);
                worker_runner.main_loop();
                ::_exit(EXIT_FAILURE);
            }
			guarantee_err(res != -1, "could not fork worker process");
            scoped_fd_t closer(worker_socket);
#endif
        }
    }

private:
    scoped_fd_t socket;
};

extproc_spawner_t::extproc_spawner_t() :
    spawner_pid(process_ref_t::invalid)
{
    // TODO: guarantee we aren't in a thread pool
    guarantee(instance == NULL);
    instance = this;

    fork_spawner();
}

extproc_spawner_t::~extproc_spawner_t() {
    // TODO: guarantee we aren't in a thread pool
    guarantee(instance == this);
    instance = NULL;

    // This should trigger the spawner to exit
    spawner_socket.reset();

    // Wait on the spawner's return value to clean up the process
#ifdef _WIN32
	WaitForSingleObject(spawner_pid, INFINITE);
#else
    int status;
    int res = waitpid(spawner_pid, &status, 0);
    guarantee_err(res == spawner_pid, "failed to wait for extproc spawner process to exit");
#endif
}

extproc_spawner_t *extproc_spawner_t::get_instance() {
    return instance;
}

void extproc_spawner_t::fork_spawner() {
    guarantee(spawner_socket.get() == INVALID_FD);
#ifdef _WIN32
	rassert(false, "ATN TODO");
#else
    fd_t fds[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    guarantee_err(res == 0, "could not create socket pair for spawner process");

    res = ::fork();
    if (res == 0) {
        // This is the spawner process, just instantiate ourselves and handle requests
        do {
            res = ::close(fds[0]);
        } while (res == 0 && get_errno() == EINTR);

        spawner_run_t spawner(fds[1]);
        spawner.main_loop();
        ::_exit(EXIT_SUCCESS);
    }

    spawner_pid = res;
    guarantee_err(spawner_pid != -1, "could not fork spawner process");

    scoped_fd_t closer(fds[1]);
    spawner_socket.reset(fds[0]);
#endif
}

// Spawns a new worker process and returns the fd of the socket used to communicate with it
fd_t extproc_spawner_t::spawn(object_buffer_t<socket_stream_t> *stream_out, process_ref_t *pid_out) {
#ifdef _WIN32
	rassert(false, "ATN TODO");
	return fd_t();
#else
    guarantee(spawner_socket.get() != INVALID_FD);

    fd_t fds[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    guarantee_err(res == 0, "could not create socket pair for worker process");

    res = send_fds(spawner_socket.get(), 1, &fds[1]);
    guarantee_err(res == 0, "could not send socket file descriptor to worker process");

    stream_out->create(fds[0], reinterpret_cast<fd_watcher_t*>(NULL));

    // Get the pid of the new worker process
    archive_result_t archive_res;
    archive_res = deserialize<cluster_version_t::LATEST_OVERALL>(stream_out->get(),
                                                                 pid_out);
    guarantee_deserialization(archive_res, "pid_out");
    guarantee(*pid_out != -1);

    scoped_fd_t closer(fds[1]);
    return fds[0];
#endif
}
