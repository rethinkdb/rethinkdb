// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef _WIN32

#include <sys/wait.h>
#include <sys/time.h>
#include <sys/socket.h>
#else
#include <atomic>
#endif
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "arch/process.hpp"
#include "extproc/extproc_spawner.hpp"
#include "extproc/extproc_worker.hpp"
#include "arch/fd_send_recv.hpp"

extproc_spawner_t *extproc_spawner_t::instance = nullptr;

// This is the class that runs in the external process, doing all the work
class worker_run_t {
public:
    worker_run_t(fd_t _socket, process_id_t _spawner_pid) :
        socket(_socket), socket_stream(socket.get(), make_scoped<blocking_fd_watcher_t>()) {

#ifdef _WIN32
        // TODO WINDOWS: make sure the worker process gets killed
#else
        guarantee(spawner_pid == INVALID_PROCESS_ID);
        spawner_pid = _spawner_pid;

        // Set ourselves to get interrupted when our parent dies
        struct sigaction sa = make_sa_handler(0, check_ppid_for_death);
        const int sigaction_res = sigaction(SIGALRM, &sa, nullptr);
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
        // Send our pid over to the main process (because it didn't fork us directly)
        write_message_t wm;
        serialize<cluster_version_t::LATEST_OVERALL>(&wm, getpid());
        int res = send_write_message(&socket_stream, &wm);
        guarantee(res == 0);
#endif
    }

    ~worker_run_t() {
#ifndef _WIN32
        guarantee(spawner_pid != INVALID_PROCESS_ID);
        spawner_pid = INVALID_PROCESS_ID;
#endif
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
#ifndef _WIN32
    static pid_t spawner_pid;

    static void check_ppid_for_death(int) {
        pid_t ppid = getppid();
        if (spawner_pid != -1 && spawner_pid != ppid) {
            ::_exit(EXIT_FAILURE);
        }
    }
#endif

    scoped_fd_t socket;
    socket_stream_t socket_stream;
};

#ifndef _WIN32
pid_t worker_run_t::spawner_pid = -1;
#endif

#ifndef _WIN32
class spawner_run_t {
public:
    explicit spawner_run_t(fd_t _socket) :
        socket(_socket) {
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
            const int res = sigaction(SIGCHLD, &sa, nullptr);
            guarantee_err(res == 0, "spawner: Could not ignore SIGCHLD");
        }
    }

    void main_loop() {
        process_id_t spawner_pid = current_process();

        while(true) {
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
        }
    }

private:
    scoped_fd_t socket;
};
#endif

extproc_spawner_t::extproc_spawner_t() {
    // TODO: guarantee we aren't in a thread pool
    guarantee(instance == nullptr);
    instance = this;

#ifdef _WIN32
    // TODO WINDOWS: ensure the workers die if the parent process does,
    // perhaps using CreateJobObject and JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
#else
    spawner_pid = INVALID_PROCESS_ID;
    fork_spawner();
#endif
}

extproc_spawner_t::~extproc_spawner_t() {
    // TODO: guarantee we aren't in a thread pool
    guarantee(instance == this);
    instance = nullptr;

#ifdef _WIN32
    // TODO WINDOWS: cleanup worker processes
#else
    // This should trigger the spawner to exit
    spawner_socket.reset();

    // Wait on the spawner's return value to clean up the process
    int status;
    int res = waitpid(spawner_pid, &status, 0);
    guarantee_err(res == spawner_pid, "failed to wait for extproc spawner process to exit");
#endif
}

extproc_spawner_t *extproc_spawner_t::get_instance() {
    return instance;
}

#ifndef _WIN32
void extproc_spawner_t::fork_spawner() {
    guarantee(spawner_socket.get() == INVALID_FD);

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
}
#endif

// Spawns a new worker process and returns the fd of the socket used to communicate with it
fd_t extproc_spawner_t::spawn(process_id_t *pid_out) {
#ifdef _WIN32
    static std::atomic<uint64_t> unique = 0;

    char rethinkdb_path[MAX_PATH];
    DWORD res = GetModuleFileName(NULL, rethinkdb_path, sizeof(rethinkdb_path));
    guarantee_winerr(res != 0 && res != sizeof(rethinkdb_path), "GetModuleFileName failed");

    scoped_fd_t fd;
    std::string pipe_path = strprintf("\\\\.\\pipe\\rethinkdb-worker-%d-%d", GetCurrentProcessId(), ++unique);
    fd.reset(CreateNamedPipe(pipe_path.c_str(),
                             PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                             PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                             1,
                             2048,
                             2048,
                             0,
                             nullptr));
    guarantee_winerr(fd.get() != INVALID_FD, "CreateNamedPipe failed");

    std::string command_line = strprintf("RethinkDB " SUBCOMMAND_START_WORKER " %s", pipe_path.c_str());
    std::vector<char> mutable_command_line(command_line.begin(), command_line.end());
    mutable_command_line.push_back('\0');

    STARTUPINFO startup_info;
    memset(&startup_info, 0, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info;
    BOOL res2 = CreateProcess(rethinkdb_path,
                              &mutable_command_line[0],
                              nullptr,
                              nullptr,
                              false,
                              NORMAL_PRIORITY_CLASS,
                              nullptr,
                              nullptr,
                              &startup_info,
                              &process_info);

    guarantee_winerr(res2, "CreateProcess failed");

    // TODO WINDOWS: add new process to job group

    *pid_out = process_id_t(GetProcessId(process_info.hProcess));
    CloseHandle(process_info.hThread);
    return fd.release();
#else
    guarantee(spawner_socket.get() != INVALID_FD);

    fd_t fds[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    guarantee_err(res == 0, "could not create socket pair for worker process");

    res = send_fds(spawner_socket.get(), 1, &fds[1]);
    guarantee_err(res == 0, "could not send socket file descriptor to worker process");

    socket_stream_t stream_out(fds[0]);

    // Get the pid of the new worker process
    archive_result_t archive_res;
    archive_res = deserialize<cluster_version_t::LATEST_OVERALL>(&stream_out,
                                                                 pid_out);
    guarantee_deserialization(archive_res, "pid_out");
    guarantee(*pid_out != INVALID_PROCESS_ID);

    scoped_fd_t closer(fds[1]);
    return fds[0];
#endif
}

#ifdef _WIN32
bool extproc_maybe_run_worker(int argc, char **argv) {
    if (argc != 3 || strcmp(argv[1], SUBCOMMAND_START_WORKER)) {
        return false;
    }

    // Don't handle signals like ^C in the extproc worker process
    SetConsoleCtrlHandler(nullptr, true);

    fd_t fd = CreateFile(argv[2],
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         nullptr,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         nullptr);
    guarantee_winerr(fd != INVALID_FD, "opening '%s'", argv[2]);

    worker_run_t worker(fd, INVALID_PROCESS_ID);
    worker.main_loop();
    ::_exit(EXIT_SUCCESS);
    return true;
}

#endif
