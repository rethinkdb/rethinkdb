// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/bind.hpp>

#include "unittest/unittest_utils.hpp"

#include "containers/archive/archive.hpp"
#include "extproc/job.hpp"
#include "extproc/pool.hpp"
#include "extproc/spawner.hpp"
#include "rpc/serialize_macros.hpp"
#include "unittest/gtest.hpp"

// ----- Infrastructure
typedef void (*test_t)(extproc::pool_t *pool);

static void run_extproc_test(extproc::spawner_t::info_t *spawner_info, test_t func) {
    extproc::pool_group_t pool_group(spawner_info, extproc::pool_group_t::DEFAULTS);
    func(pool_group.get());
}

static void main_extproc_test(test_t func) {
    extproc::spawner_t::info_t spawner_info;
    extproc::spawner_t::create(&spawner_info);
    unittest::run_in_thread_pool(boost::bind(run_extproc_test, &spawner_info, func));
}

int fib(int n) {
    int a = 0, b = 1;
    if (!n) return 0;
    while (--n) {
        int next = a + b;
        a = b;
        b = next;
    }
    return b;
}

int collatz(int n) {
    return (n & 1) ? 3 * n + 1 : n / 2;
}

// ----- Jobs
struct fib_job_t : extproc::auto_job_t<fib_job_t> {
    // Calculates the nth fibonacci number.
    fib_job_t() {}
    explicit fib_job_t(int n) : n_(n) {}

    int n_;
    RDB_MAKE_ME_SERIALIZABLE_1(n_);

    virtual void run_job(control_t *control, UNUSED void *extra) {
        int res = fib(n_);
        write_message_t msg;
        msg << res;
        const int write_res = send_write_message(control, &msg);
        guarantee(0 == write_res);
    }
};

struct collatz_job_t : extproc::auto_job_t<collatz_job_t> {
    // Streams the collatz numbers starting at the given number, until it hits
    // 1. Waits for a command to proceed in between each result.
    //
    // NB. The iterated collatz function has been shown to terminate for numbers
    // <= 10 billion.
    collatz_job_t() {}
    explicit collatz_job_t(int n) : n_(n) {}

    void run_job(control_t *control, UNUSED void *extra) {
        for (;;) {
            // Send current value.
            write_message_t msg;
            msg << n_;
            const int write_res = send_write_message(control, &msg);
            guarantee(0 == write_res);

            // We're done once we hit 1.
            if (n_ == 1) {
                break;
            }

            // Wait for signal to proceed.
            char c;
            const int64_t read_res = force_read(control, &c, 1);
            guarantee(1 == read_res);

            n_ = collatz(n_);
        }
    }

    RDB_MAKE_ME_SERIALIZABLE_1(n_);

private:
    int n_;

    DISABLE_COPYING(collatz_job_t);
};

struct job_loop_t : extproc::auto_job_t<job_loop_t> {
    // Receives a job and runs it.
    job_loop_t() {}

    RDB_MAKE_ME_SERIALIZABLE_0();

    void run_job(control_t *control, UNUSED void *extra) {
        // Loops accepting jobs until we tell it to quit.
        for (;;) {
            bool quit;
            const archive_result_t res = deserialize(control, &quit);
            guarantee(res == ARCHIVE_SUCCESS);
            if (quit) {
                break;
            }
            const int accept_res = extproc::job_t::accept_job(control, NULL);
            guarantee(0 == accept_res);
        }

        // Sends signal that it has quit.
        const int64_t write_res = control->write("done", 4);
        guarantee(4 == write_res);
    }
};

// ----- Tests
void run_simplejob_test(extproc::pool_t *pool) {
    // Calculates fib(10).
    extproc::job_handle_t handle;
    handle.begin(pool, fib_job_t(10));

    int job_result;
    const archive_result_t res = deserialize(&handle, &job_result);
    ASSERT_EQ(ARCHIVE_SUCCESS, res);
    ASSERT_EQ(fib(10), job_result);

    handle.release();
}

TEST(ExtProc, SimpleJob) { main_extproc_test(run_simplejob_test); }

void run_talkativejob_test(extproc::pool_t *pool) {
    int n = 78;                 // takes 35 iterations to reach 1
    extproc::job_handle_t handle;
    const int begin_res = handle.begin(pool, collatz_job_t(n));
    ASSERT_EQ(0, begin_res);

    for (;; n = collatz(n)) {
        int job_result;
        // Get & check streamed result.
        const archive_result_t res = deserialize(&handle, &job_result);
        ASSERT_EQ(ARCHIVE_SUCCESS, res);
        ASSERT_EQ(n, job_result);

        if (job_result == 1) break;

        // Send signal to continue.
        const char c = '\0';
        const int64_t write_res = handle.write(&c, 1);
        ASSERT_EQ(1, write_res);
    }

    handle.release();
}

TEST(ExtProc, TalkativeJob) { main_extproc_test(run_talkativejob_test); }

void run_serialjob_test(extproc::pool_t *pool) {
    extproc::job_handle_t handle;
    int begin_res = handle.begin(pool, job_loop_t());
    ASSERT_EQ(0, begin_res);

    // Run fib three times.
    const int inputs[] = {10, 11, 12};

    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        const int n = inputs[i];
        SCOPED_TRACE(strprintf("fib(%d)", n));

        {
            write_message_t msg;
            msg << false;
            fib_job_t(n).append_to(&msg);
            const int res = send_write_message(&handle, &msg);
            ASSERT_EQ(0, res);
        }

        int job_result;
        const archive_result_t res = deserialize(&handle, &job_result);
        ASSERT_EQ(ARCHIVE_SUCCESS, res);
        ASSERT_EQ(fib(n), job_result);
    }

    {
        // Tell it to stop running jobs.
        write_message_t msg;
        msg << true;
        const int res = send_write_message(&handle, &msg);
        ASSERT_EQ(0, res);
    }

    char msg[4];
    int64_t read_res = force_read(&handle, msg, sizeof(msg));
    ASSERT_EQ(static_cast<int64_t>(sizeof(msg)), read_res);
    ASSERT_EQ(0, memcmp(msg, "done", sizeof(msg)));

    handle.release();
}

TEST(ExtProc, SerialJob) { main_extproc_test(run_serialjob_test); }

void run_interruptjob_test(extproc::pool_t *pool) {
    const int n = 78;           // takes 35 iterations to reach 1
    extproc::job_handle_t handle;

    // Interrupt DEFAULT_MIN_WORKERS + 1 jobs, to check that the pool respawns
    // interrupted workers as necessary.
    for (int i = 0; i < extproc::pool_group_t::DEFAULT_MIN_WORKERS + 1; ++i) {
        const int begin_res = handle.begin(pool, collatz_job_t(n));
        ASSERT_EQ(0, begin_res);

        // Run a few iterations.
        for (int j = 0; j < 10; ++j) {
            int job_result;
            const archive_result_t res = deserialize(&handle, &job_result);
            ASSERT_EQ(ARCHIVE_SUCCESS, res);
            ASSERT_NE(1, job_result);

            const char c = '\0';
            const int64_t write_res = handle.write(&c, 1);
            ASSERT_EQ(1, write_res);
        }

        // Interrupt the job.
        handle.interrupt();
    }
}

TEST(ExtProc, InterruptJob) { main_extproc_test(run_interruptjob_test); }

void run_multijob_test(extproc::spawner_t::info_t *spawner_info) {
    extproc::pool_group_t::config_t config;
    // Ensure that we have the headroom to spawn an extra worker.
    const int njobs = config.max_workers = config.min_workers + 1;

    extproc::pool_group_t pool_group(spawner_info, config);
    extproc::pool_t *pool = pool_group.get();

    scoped_array_t<extproc::job_handle_t> handles(njobs);

    // Spawn off enough jobs that we need more than the minimum number of
    // workers to service them all at once, to check that the pool will spawn
    // another.
    const fib_job_t job(10);
    for (int i = 0; i < njobs; ++i) {
        const int begin_res = handles[i].begin(pool, job);
        ASSERT_EQ(0, begin_res);
    }

    // Finish the jobs.
    int expect = fib(10);
    for (int i = 0; i < njobs; ++i) {
        int job_result;
        const archive_result_t res = deserialize(&handles[i], &job_result);
        ASSERT_EQ(ARCHIVE_SUCCESS, res);
        ASSERT_EQ(expect, job_result);
        handles[i].release();
    }
}

TEST(ExtProc, MultiJob) {
    extproc::spawner_t::info_t spawner_info;
    extproc::spawner_t::create(&spawner_info);
    unittest::run_in_thread_pool(boost::bind(run_multijob_test, &spawner_info));
}
