#include "extproc/pool.hpp"

#include "containers/scoped.hpp"
#include "logger.hpp"
#include "rpc/serialize_macros.hpp"

namespace extproc {

// ---------- pool_group_t ----------
const pool_group_t::config_t pool_group_t::DEFAULTS;

pool_group_t::pool_group_t(spawner_t::info_t *info, const config_t &config)
    : spawner_(info), config_(config),
      pool_maker_(this)
{
    // Check config for sanity
    guarantee(config_.max_workers >= config_.min_workers &&
              config_.max_workers > 0);
}


// ---------- pool_t ----------
pool_t::pool_t(pool_group_t *group)
    : group_(group),
      num_spawning_workers_(0),
      worker_semaphore_(group_->config_.max_workers)
{
    // Spawn initial worker pool.
    repair_invariants();
}

pool_t::~pool_t() {
    // All workers should be idle before we shut down (clients can interrupt
    // them if necessary).
    guarantee(busy_workers_.empty(), "Busy workers at pool shutdown!");

    // Kill worker processes.
    worker_t *w;
    while ((w = idle_workers_.head()))
        end_worker(&idle_workers_, w);
}

void pool_t::repair_invariants() {
    int need = config()->min_workers - num_workers();
    if (need > 0)
        spawn_workers(need);

    // Double-check.
    rassert(num_workers() >= config()->min_workers);
}

pool_t::worker_t *pool_t::acquire_worker() {
    assert_thread();

    // We're going to be using up a worker process, so we lock the semaphore.
    // This blocks if `config()->max_workers` workers are currently in use. We
    // unlock the semaphore in disconnect(), once we're done using the worker
    // process.
    worker_semaphore_.co_lock();

    // An `if` would suffice here in place of a `while`. However, this is only
    // because spawn_workers() performs no blocking/yielding operations between
    // when it adds the created workers to the idle_workers_ list and when it
    // returns. Otherwise, someone could call connect() and 'steal' our spawned
    // worker in the mean time. In case this changes later, we use a `while`.
    while (idle_workers_.empty()) {
        // Our usage of worker_semaphore_ should ensure this.
        rassert(num_workers() < config()->max_workers);
        spawn_workers(1);
    }
    guarantee(!idle_workers_.empty()); // sanity

    // Grab an idle worker, move it to the busy list, assign it to `handle`.
    worker_t *worker = idle_workers_.head();
    idle_workers_.remove(worker);
    busy_workers_.push_back(worker);
    return worker;
}

void pool_t::release_worker(worker_t *worker) THROWS_NOTHING {
    assert_thread();
    rassert(worker && worker->pool_ == this);

    // If the worker's stream isn't open, something bad has happened.
    if (!worker->is_read_open() || !worker->is_write_open()) {
        worker->on_error();

        // TODO(rntz): Currently worker_t::on_error() never returns. If we ever
        // change this, some code needs to get written here.
        unreachable();
    }

    // Move it from the busy list to the idle list.
    busy_workers_.remove(worker);
    idle_workers_.push_back(worker);

    // Free up worker slot for someone else to use.
    worker_semaphore_.unlock();
}

void pool_t::interrupt_worker(worker_t *worker) THROWS_NOTHING {
    assert_thread();
    rassert(worker && worker->pool_ == this);

    end_worker(&busy_workers_, worker);
    worker_semaphore_.unlock();

    repair_invariants();
}

void pool_t::detach_worker(worker_t *worker) {
    rassert(worker && worker->pool_ == this && worker->attached_);
    ASSERT_NO_CORO_WAITING;

    worker->attached_ = false;
    busy_workers_.remove(worker);
    worker_semaphore_.unlock();

    guarantee_err(0 ==  kill(worker->pid_, SIGKILL), "could not kill worker");

    // Alas, we can't call repair_invariants now, since we're not allowed to
    // block.
}

void pool_t::cleanup_detached_worker(worker_t *worker) {
    rassert(worker && worker->pool_ == this && !worker->attached_);
    delete worker;
    repair_invariants();
}

// The root job that runs on spawned worker processes.
class job_acceptor_t :
        public auto_job_t<job_acceptor_t>
{
  public:
    void run_job(control_t *control, UNUSED void *extra) {
        while (-1 != accept_job(control, NULL)) {}

        // The "correct" way for us to die is to be killed by the engine
        // process. So if we reach this, something is wrong.
        control->log("Failed to accept job, quitting.");
        exit(EXIT_FAILURE);
    }

    RDB_MAKE_ME_SERIALIZABLE_0();
};

void pool_t::spawn_workers(int num) {
    assert_thread();
    guarantee(num > 0);

    num_spawning_workers_ += num;
    guarantee(num_workers() <= config()->max_workers);

    // Spawn off `num` processes.
    pid_t pids[num];
    scoped_array_t<scoped_fd_t> fds(num);
    {
        on_thread_t switcher(spawner()->home_thread());
        for (int i = 0; i < num; ++i) {
            pids[i] = spawner()->spawn_process(&fds[i]);
            guarantee(-1 != pids[i], "could not spawn worker process");
        }
    }

    // For every process spawned, create a corresponding worker_t.
    for (int i = 0; i < num; ++i) {
        worker_t *worker = new worker_t(this, pids[i], &fds[i]);

        // Send it a job that just loops accepting jobs.
        guarantee(0 == job_acceptor_t().send_over(worker),
                  "Could not initialize worker process.");

        // We've successfully spawned one worker.
        guarantee(num_spawning_workers_ > 0); // sanity
        --num_spawning_workers_;
        idle_workers_.push_back(worker);
    }
}

// May only call when we're sure that the worker is not already dead.
void pool_t::end_worker(workers_t *list, worker_t *worker) {
    rassert(worker && worker->pool_ == this);

    list->remove(worker);
    guarantee_err(0 ==  kill(worker->pid_, SIGKILL), "could not kill worker");
    delete worker;
}


// ---------- pool_t::worker_t ----------
pool_t::worker_t::worker_t(pool_t *pool, pid_t pid, scoped_fd_t *fd)
    : unix_socket_stream_t(fd),
      pool_(pool), pid_(pid),
      attached_(true)
{
    guarantee(pid > 1 && fd > 0); // sanity
}

pool_t::worker_t::~worker_t() {}

void pool_t::worker_t::on_event(UNUSED int events) {
    // NB. We are not in coroutine context when this method is called.
    if (attached_) {
        on_error();
    }
    unix_socket_stream_t::on_event(events);
}

void pool_t::worker_t::on_error() {
    // NB. We may or may not be in coroutine context when this method is called.
    assert_thread();

    // We got an error on our socket to this worker. This shouldn't happen.
    crash_or_trap("Error on worker process socket");

    // TODO(rntz): Arguably we should be more robust. If someone kills one of
    // our worker processes, we may not want the entire rethinkdb instance to
    // die.
    //
    // In any case, we certainly want to report an error somehow.
}


// ---------- job_handle_t ----------
job_handle_t::job_handle_t()
    : worker_(NULL) {}

job_handle_t::~job_handle_t() {
    if (connected()) {
        // Please call interrupt() explicitly. This is here so that you don't
        // have to write try/finally every time you use a job handle in a
        // context where exceptions might be raised.
        logWRN("job handle still connected on destruction");
        debugf("job handle still connected on destruction, interrupting\n");
        interrupt();
    }
    rassert(!connected(), "job handle still connected on destruction");
}

int job_handle_t::begin(pool_t *pool, const job_t &job) {
    rassert(!connected());

    worker_ = pool->acquire_worker();
    guarantee(worker_);         // for now, acquiring worker can't fail

    int res = job.send_over(this);
    // If sending failed, write() should have disconnected us.
    rassert(0 == res ? connected() : !connected());
    return res;
}

void job_handle_t::release() THROWS_NOTHING {
    rassert(connected() && worker_->attached_);
    worker_->pool_->release_worker(worker_);
    worker_ = NULL;
}

void job_handle_t::interrupt() THROWS_NOTHING {
    rassert(connected() && worker_->attached_);
    worker_->pool_->interrupt_worker(worker_);
    worker_ = NULL;
}

int64_t job_handle_t::read_interruptible(void *p, int64_t n, signal_t *interruptor) {
    rassert(worker_ && worker_->attached_);
    int res;
    try {
        interruptor_wrapper_t wrapper(this, interruptor);
        res = worker_->read_interruptible(p, n, &wrapper);
    } catch (interrupted_exc_t) {
        // We were interrupted, and need to clean up the detached worker created
        // by job_handle_t::interruptor_wrapper_t::run(). We do this by falling
        // through to check_attached(), which will re-raise an interrupted_exc_t
        // for us. The reason we don't just handle it here is that it isn't safe
        // to do anything that might block in a catch() block.
        rassert(worker_ && !worker_->attached_);
    }
    check_attached();

    if (-1 == res || 0 == res) release();
    return res;
}

int64_t job_handle_t::write_interruptible(const void *p, int64_t n, signal_t *interruptor) {
    rassert(worker_ && worker_->attached_);
    int res;
    try {
        interruptor_wrapper_t wrapper(this, interruptor);
        res = worker_->write_interruptible(p, n, &wrapper);
    } catch (interrupted_exc_t) {
        // See comments in read_interruptible.
        rassert(worker_ && !worker_->attached_);
    }
    check_attached();

    if (-1 == res) release();
    return res;
}

void job_handle_t::check_attached() {
    if (worker_ && !worker_->attached_) {
        worker_->pool_->cleanup_detached_worker(worker_);
        worker_ = NULL;
        throw interrupted_exc_t();
    }
}

// ---------- job_handle_t::interruptor_wrapper_t ----------
job_handle_t::interruptor_wrapper_t::interruptor_wrapper_t(job_handle_t *handle, signal_t *signal)
        : handle_(handle)
{
    reset(signal);          // might call run()
}

// Inherited from signal_t::subscription_t. Called on pulse() of signal passed
// to ctor. NB. MUST NOT THROW. Could eg. be called from interruptor_wrapper_t
// constructor.
void job_handle_t::interruptor_wrapper_t::run() THROWS_NOTHING {
    signal_t::assert_thread();
    rassert(!is_pulsed());  // sanity

    // Ignore errors on the worker socket. This is necessary because
    // interruption causes the worker socket to shutdown (see
    // socket_stream_t::wait_for_{read,write}). When the worker process notices
    // this it exits, closing its end of the socket and causing us to get an
    // error.
    //
    // We actually destroy the worker (via pool_t::cleanup_detached_worker) at
    // the end of whatever operation we're serving as an interruptor wrapper
    // for.
    handle_->worker_->pool_->detach_worker(handle_->worker_);

    pulse();
}

} // namespace extproc
