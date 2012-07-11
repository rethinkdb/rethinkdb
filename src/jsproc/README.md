# Spawner process startup

When RethinkDB is invoked, if the command requested might require running JS
code (ie. if it involves calling `serve()`; as of 2012-07-11, this means
`main_rethinkdb_porcelain()` and `main_rethinkdb_serve()`), before
`run_in_thread_pool()` is called, fork off a process by calling
`spawner_t::create()`.

Before forking, we create a pair of unix domain sockets. The parent gets one end
and the child the other. The parent process continues as usual, becoming the
RethinkDB "engine process". The child becomes a "spawner process".

The spawner loops, receiving file descriptors from the engine (unix domain
sockets can send & receive open file descriptors). For each fd received, it
forks off a worker process with that fd, and sends the worker's PID back to the
engine.

The new worker receives a single "job" (see below) from the engine over this fd
and runs it. The job can use the fd to communicate with the engine.

Note that although we `fork()`, we never call `exec()` or a variant: all
processes involved are images of the same RethinkDB binary.

In sum, there are three kinds of process: the "engine" parent process, the
"spawner" child process, and the "worker" grandchild processes.


# Jobs and pools

A "job" is a unit of work that runs in a worker process and communicates with
the engine via a unix socket. It must be serializable so we can send it to the
worker.

The spawner is very simple: it spawns a worker running a single job when the
engine requests it. The worker dies when the job completes. Pools wrap the
spawner to provide pooling of workers, multiplexing of jobs, and error condition
handling.

Each thread gets its own pool of workers. The "initial job" that a pool's
workers are spawned with is a "job-running job": it loops accepting jobs from
the engine and running them.


# Death handling

Each process (engine, spawner, workers) should appropriately handle the death of
other processes.

## Engine death

The engine is the core of RethinkDB; if it dies (whether unexpectedly or by
normal shut-down), everything should.

The spawner loops reading/writing to the engine; if a read or write fails, it
exits.

The workers also exit if communication (read/write) with the engine fails.
However, workers could be busy and not trying to communicate with the engine. In
this case, they will die via SIGTERM when the spawner dies (see below).

## Spawner death

The spawner is a simple process that shouldn't die except by programmer error or
external intervention. It also can't be replaced if it does die. So if it dies
(except as part of normal shut-down), RethinkDB should die.

The engine sets up a SIGCHLD handler that crashes, so if the spawner dies
unexpectedly, the engine dies. The handler is de-registered during normal
shut-down, to avoid crashing when the spawner correctly exits.

The workers call `prctl(PR_SET_PDEATHSIG, SIGTERM)` (alas, Linux-specific), so
they receive SIGTERM if the spawner (their parent) dies.

## Worker death

Unplanned worker process death is not entirely unexpected: v8 might have a bug,
or the JS code might use up too much memory and get OOM killed. It's not
entirely clear what we should do in this case.

The engine watches all its connections to worker processes (via
`pool_t::worker_t`) for errors (see pool.cc). If any fail, either when idle or
during a read or write, it calls `worker_t::on_error()`. Currently, this crashes
the engine process.

The spawner pays no particular attention to death of its children (indeed, it
ignores SIGCHLD so as not to leave zombies hanging around); its job is to spawn
things and forget about them. (Of course, when the engine crashes due to a
worker failure, the spawner will die in response.)
