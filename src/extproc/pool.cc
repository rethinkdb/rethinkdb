#include "extproc/pool.hpp"

#include "utils.hpp"
#include <boost/scoped_array.hpp>

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

int pool_t::spawn_job(job_handle_t *handle, const job_t &job) {
    worker_t *worker = acquire_worker();
    guarantee(worker);          // for now, acquiring worker can't fail
    handle->connect(worker);
    int res = job.send_over(handle);
    // If sending failed, the handle should have disconnected.
    rassert(0 == res || !handle->connected());
    return res;
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

void pool_t::release_worker(worker_t *worker) {
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

void pool_t::interrupt_worker(worker_t *worker) {
    assert_thread();
    rassert(worker && worker->pool_ == this);

    end_worker(&busy_workers_, worker);
    worker_semaphore_.unlock();

    repair_invariants();
}

// The root job that runs on spawned worker processes.
class job_acceptor_t :
        public auto_job_t<job_acceptor_t>
{
  public:
    void run_job(control_t *control) {
        while (-1 != accept_job(control)) {}

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
    boost::scoped_array<scoped_fd_t> fds(new scoped_fd_t[num]);
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

    guarantee_err(0 ==  kill(worker->pid_, SIGKILL), "could not kill worker");
    list->remove(worker);
    delete worker;
}


// ---------- pool_t::worker_t ----------
pool_t::worker_t::worker_t(pool_t *pool, pid_t pid, scoped_fd_t *fd)
    : unix_socket_stream_t(fd),
      pool_(pool), pid_(pid)
{
    guarantee(pid > 1 && fd > 0); // sanity
}

pool_t::worker_t::~worker_t() {}

void pool_t::worker_t::on_event(UNUSED int events) {
    // NB. We are not in coroutine context when this method is called.
    on_error();
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

void job_handle_t::connect(pool_t::worker_t *worker) {
    rassert(!connected() && worker);
    worker_ = worker;
    worker_->assert_thread();
}

void job_handle_t::release() {
    rassert(connected());
    worker_->release();
    worker_ = NULL;
}

void job_handle_t::interrupt() {
    rassert(connected());
    worker_->interrupt();
    worker_ = NULL;
}

} // namespace extproc
