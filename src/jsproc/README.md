# Supervisor process startup

When rethinkdb is invoked, if the command requested might require running JS
code (ie. if it involves calling `serve()`; `main_rethinkdb_porcelain()` and
`main_rethinkdb_serve()` are the two entry points as of this writing), before
the `run_in_thread_pool()` is called, we spawn off a supervisor process by
calling `supervisor_t::spawn()`.

`supervisor_t::spawn()` calls `fork()`. On failure, we die. On success,
`supervisor_t::spawn()` returns _in the child process only_. In the parent, it
runs `exec_supervisor()` instead, which does not return.

The parent process becomes a "supervisor process" whose job it is to manage the
RethinkDB engine process and a pool of worker processes (in which JS is run).
The worker processes are themselves forked off from the supervisor, and run
`exec_worker()`. Note that although we `fork()`, we never call `exec()` or a
variant: all processes involved are images of the same RethinkDB binary.

In sum, there are three kinds of process: the "supervisor" parent process, the
"engine" child process (in which `supervisor_t::spawn()` returns), and the
"worker" side-processes.

# Communication between supervisor, engine, and workers

Before forking the engine process, the supervisor calls `socketpair()` to open a
socket which is used to communicate with the engine process. The same is done
for the worker processes. So the supervisor has a socket for each child process,
and each child has a socket connected to the supervisor.

The supervisor keeps worker processes in two lists, idle and busy. Initially all
workers are idle.

When the engine wishes to connect to a worker, it creates a `socketpair()` and
sends one end over its connection to the supervisor process. On receiving a file
descriptor `fd` from the engine, the supervisor picks an idle worker and sends
`fd` to that worker, moving it to the "busy" list. The engine and worker then
communicate directly over the socket pair they now share.

Once the engine and worker are done communicating, they close their socket pair.
The worker then sends the supervisor a notification that it is idle again.

# Job execution

(See the code in `worker_proc.cc`.)

Worker processes run a loop which waits for the supervisor to send them a file
descriptor (one end of a connection to the rethinkdb engine). When they get a
file descriptor, they attempt to read a job from it and execute it.

To receive a job, first they read a function pointer. This is feasible because
they are forks of the rethinkdb process, so function addresses are the same
between them, the supervisor, and the engine. The worker calls the function
pointer, giving it a handle to the file descriptor it received.

# Potential pain points / decisions needing revision

- The supervisor uses `select()` to multiplex IO. This may not scale if we need
  lots and lots of worker processes.

- The supervisor tries to ensure that there are exactly a certain number of
  worker processes. If one dies, an message is logged to stderr, and a new
  worker is forked to replace it. See `jsproc/supervisor_proc.cc` for the
  `#define`s that control the logic behind this.

- Logging of errors just goes to stderr, since it would be highly nontrivial to
  hook in to the main RethinkDB logging system.

- Killing the supervisor process won't kill the children. Idle workers will die
  when they try to get a new job from the supervisor, but the rethinkdb engine
  will probably continue oblivious.

  (The supervisor *does* handle shut-down of the rethinkdb engine process
  correctly, by killing the worker processes and exiting.)
