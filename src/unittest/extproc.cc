// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/bind.hpp>

#include "unittest/unittest_utils.hpp"

#include "arch/runtime/coroutines.hpp"
#include "containers/archive/archive.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_spawner.hpp"
#include "extproc/extproc_job.hpp"
#include "rpc/serialize_macros.hpp"
#include "unittest/gtest.hpp"

uint64_t fib(size_t iterations) {
    uint64_t a = 0, b = 1;
    if (iterations == 0) return 0;
    while (--iterations > 0) {
        int next = a + b;
        a = b;
        b = next;
    }
    return b;
}

uint64_t collatz(uint64_t n) {
    return (n & 1) ? 3 * n + 1 : n / 2;
}

// Calculates the nth fibonacci number.
class fib_job_t {
public:
    fib_job_t(size_t n, extproc_pool_t *pool, signal_t *interruptor) :
        extproc_job(pool, &worker_fn, interruptor),
        iterations(n) { }

    uint64_t run() {
        write_message_t wm;
        wm << iterations;
        int res = send_write_message(extproc_job.write_stream(), &wm);
        guarantee(res == 0);

        uint64_t result;
        res = deserialize(extproc_job.read_stream(), &result);
        guarantee(res == ARCHIVE_SUCCESS);
        return result;
    }

private:
    static bool worker_fn(read_stream_t *stream_in, write_stream_t *stream_out) {
        size_t count;
        int res = deserialize(stream_in, &count);
        guarantee(res == ARCHIVE_SUCCESS);

        uint64_t result = fib(count);
        write_message_t wm;
        wm << result;
        res = send_write_message(stream_out, &wm);
        guarantee(res == 0);
        return true;
    }

    extproc_job_t extproc_job;
    size_t iterations;
};

class collatz_job_t {
public:
    // Streams the collatz numbers starting at the given number, until it hits
    // 1. Waits for a command to proceed in between each result.
    //
    // NB. The iterated collatz function has been shown to terminate for numbers
    // <= 10 billion.
    collatz_job_t(uint64_t n, extproc_pool_t *pool, signal_t *interruptor) :
        extproc_job(pool, &worker_fn, interruptor),
        last_value(n) {
        // Kick off the worker with the initial value
        write_message_t wm;
        wm << last_value;
        int res = send_write_message(extproc_job.write_stream(), &wm);
        guarantee(res == 0);
    }

    ~collatz_job_t() {
        // Read out the last number
        uint64_t res = deserialize(extproc_job.read_stream(), &last_value);
        guarantee(res == ARCHIVE_SUCCESS);

        // Tell the worker to exit
        write_message_t wm;
        wm << 'q';
        res = send_write_message(extproc_job.write_stream(), &wm);
        guarantee(res == 0);
    }

    uint64_t step() {
        uint64_t res = deserialize(extproc_job.read_stream(), &last_value);
        guarantee(res == ARCHIVE_SUCCESS);

        // Send the notification to continue
        write_message_t wm;
        wm << 'c';
        res = send_write_message(extproc_job.write_stream(), &wm);
        guarantee(res == 0);
        return last_value;
    }

private:
    static bool worker_fn(read_stream_t *stream_in, write_stream_t *stream_out) {
        char command;
        uint64_t current_value;
        int res = deserialize(stream_in, &current_value);
        guarantee(res == ARCHIVE_SUCCESS);

        do {
            current_value = collatz(current_value);

            // Send current value.
            write_message_t msg;
            msg << current_value;
            res = send_write_message(stream_out, &msg);
            guarantee(res == 0);

            // Wait for signal to proceed.
            res = deserialize(stream_in, &command);
            guarantee(res == ARCHIVE_SUCCESS);
        } while (command == 'c');

        return true;
    }

    extproc_job_t extproc_job;
    uint64_t last_value;
};

void run_simple_job_test() {
    extproc_pool_t pool(2);
    fib_job_t job(10, &pool, NULL);
    uint64_t result = job.run();
    ASSERT_EQ(fib(10), result);
}

TEST(ExtProc, SimpleJob) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_simple_job_test));
}

void run_talkative_job_test() {
    extproc_pool_t pool(2);

    uint64_t n = 78;
    collatz_job_t job(n, &pool, NULL); // takes 35 iterations to reach 1

    do {
        n = collatz(n);
        uint64_t result = job.step();
        ASSERT_EQ(n, result);
    } while (n != 1);
}

TEST(ExtProc, TalkativeJob) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_talkative_job_test));
}

void run_single_job(extproc_pool_t *pool, size_t *counter, cond_t *done) {
    fib_job_t job(10, pool, NULL);

    int result = job.run();
    int expected = fib(10);

    ASSERT_EQ(expected, result);

    --*counter;
    if (*counter == 0) {
        done->pulse();
    }
}

void run_multi_job_test() {
    const size_t num_workers = 2;
    const size_t num_jobs = num_workers * 200;

    extproc_pool_t pool(num_workers);

    // Synchronization variables
    cond_t start_cond;
    cond_t done_cond;
    size_t working = num_jobs;

    // Spawn enough jobs that some have to wait for previous jobs to complete
    for (size_t i = 0; i < num_jobs; ++i) {
        coro_t::spawn_sometime(boost::bind(&run_single_job,
                                           &pool, &working, &done_cond));
    }

    // Wait for all jobs to complete
    done_cond.wait();
}

TEST(ExtProc, MultiJob) {
    for (size_t i = 0; i < 100; ++i) {
        extproc_spawner_t extproc_spawner;
        unittest::run_in_thread_pool(boost::bind(run_multi_job_test));
    }
}

class base_crash_job_t {
public:
    base_crash_job_t(extproc_pool_t *pool,
                     bool (*worker_fn) (read_stream_t *, write_stream_t *)) :
        extproc_job(pool, worker_fn, NULL) { }

    void read() {
        int data;
        int res = deserialize(extproc_job.read_stream(), &data);
        if (res != ARCHIVE_SUCCESS) {
            throw std::runtime_error("read failed");
        }
    }

    void write() {
        write_message_t msg;
        msg << 100;
        int res = send_write_message(extproc_job.write_stream(), &msg);
        if (res != 0) {
            throw std::runtime_error("read failed");
        }
    }

private:
    extproc_job_t extproc_job;
};

class exit_job_t : public base_crash_job_t {
public:
    explicit exit_job_t(extproc_pool_t *pool) :
        base_crash_job_t(pool, &worker_fn) { }

private:
    static bool worker_fn(read_stream_t *, write_stream_t *) {
        exit(1);
        return true;
    }
};

class segfault_job_t : public base_crash_job_t {
public:
    explicit segfault_job_t(extproc_pool_t *pool) :
        base_crash_job_t(pool, &worker_fn) { }

private:
    static bool worker_fn(read_stream_t *, write_stream_t *) {
        int bad_data = *reinterpret_cast<volatile int*>(NULL);
        // We should never get here
        guarantee(bad_data == 0);
        return true;
    }
};

class fail_job_t : public base_crash_job_t {
public:
    explicit fail_job_t(extproc_pool_t *pool) :
        base_crash_job_t(pool, &worker_fn) { }

private:
    static bool worker_fn(read_stream_t *, write_stream_t *) {
        return false;
    }
};

class guarantee_fail_job_t : public base_crash_job_t {
public:
    explicit guarantee_fail_job_t(extproc_pool_t *pool) :
        base_crash_job_t(pool, &worker_fn) { }

private:
    static bool worker_fn(read_stream_t *, write_stream_t *) {
        guarantee(false);
        return true;
    }
};

void run_crashed_job_test() {
    extproc_pool_t pool(1);

    // Crash the worker, then make sure it recovers and we can still run more jobs
    // First case: have the worker process exit from within the job
    {
        exit_job_t exiter(&pool);
        ASSERT_THROW(exiter.read(), std::runtime_error);
        ASSERT_THROW(exiter.write(), std::runtime_error);
    }

    for (size_t i = 0; i < 10; ++i) {
        fib_job_t job(10, &pool, NULL);
        uint64_t res = job.run();
        uint64_t expected = fib(10);
        ASSERT_EQ(res, expected);
    }

    // Second case: have the job return failure within the worker process
    {
        fail_job_t failer(&pool);
        ASSERT_THROW(failer.read(), std::runtime_error);
        ASSERT_THROW(failer.write(), std::runtime_error);
    }

    for (size_t i = 0; i < 10; ++i) {
        fib_job_t job(10, &pool, NULL);
        uint64_t res = job.run();
        uint64_t expected = fib(10);
        ASSERT_EQ(res, expected);
    }

    /* Commenting this out because it looks like a crash
    // Third case: have the worker segfault from within the job
    {
        segfault_job_t segfaulter(&pool);
        ASSERT_THROW(segfaulter.read(), std::runtime_error);
        ASSERT_THROW(segfaulter.write(), std::runtime_error);
    }
    */

    for (size_t i = 0; i < 10; ++i) {
        fib_job_t job(10, &pool, NULL);
        uint64_t res = job.run();
        uint64_t expected = fib(10);
        ASSERT_EQ(res, expected);
    }


    /* Commenting this out because it looks like a crash
    // Fourth case: have a worker guarantee failure
    {
        guarantee_fail_job_t failer(&pool);
        ASSERT_THROW(failer.read(), std::runtime_error);
        ASSERT_THROW(failer.write(), std::runtime_error);
    }
    */

    for (size_t i = 0; i < 10; ++i) {
        fib_job_t job(10, &pool, NULL);
        uint64_t res = job.run();
        uint64_t expected = fib(10);
        ASSERT_EQ(res, expected);
    }
}

TEST(ExtProc, CrashedJob) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(run_crashed_job_test));
}

class hang_job_t {
public:
    hang_job_t(extproc_pool_t *pool, signal_t *interruptor) :
        extproc_job(pool, &worker_fn, interruptor) { }

private:
    static bool worker_fn(read_stream_t *stream_in, write_stream_t *) {
        while (true) {
            int data;
            int res = deserialize(stream_in, &data);

            // We don't actually care about this, but MUST_USE...
            if (res != 0) {
                continue;
            }
        }
        guarantee(false, "worker should hang");
        return true;
    }

    extproc_job_t extproc_job;
};

void run_hanging_job_test() {
    extproc_pool_t pool(1);

    // Run a job that never returns, then make sure it recovers and we can still run more jobs
    {
        hang_job_t hanger(&pool, NULL);
    }

    for (size_t i = 0; i < 10; ++i) {
        fib_job_t job(10, &pool, NULL);
        uint64_t res = job.run();
        uint64_t expected = fib(10);
        ASSERT_EQ(res, expected);
    }
}

TEST(ExtProc, HangingJob) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(run_hanging_job_test));
}

class corrupt_job_t {
public:
    explicit corrupt_job_t(extproc_pool_t *pool) :
        extproc_job(pool, &worker_fn, NULL) { }

    void corrupt(bool direction) {
        write_message_t msg;
        msg << direction;
        int res = send_write_message(extproc_job.write_stream(), &msg);
        guarantee(res == 0);

        if (!direction) {
            res = send_write_message(extproc_job.write_stream(), &msg);
            guarantee(res == 0);
        }
    }

private:

    static bool worker_fn(read_stream_t *stream_in, write_stream_t *stream_out) {
        bool send_data = false;
        int res = deserialize(stream_in, &send_data);
        guarantee(res == 0);

        if (send_data) {
            write_message_t msg;
            msg << send_data;
            res = send_write_message(stream_out, &msg);
            guarantee(res == 0);
        }

        return true;
    }

    extproc_job_t extproc_job;
};

void run_corrupt_job_test() {
    extproc_pool_t pool(1);

    // Run a job that leaves the stream in a bad state, then make sure it recovers
    // First case: extra data in the parent->child stream
    {
        corrupt_job_t corruptor(&pool);
        corruptor.corrupt(false);
    }

    for (size_t i = 0; i < 10; ++i) {
        fib_job_t job(10, &pool, NULL);
        uint64_t res = job.run();
        uint64_t expected = fib(10);
        ASSERT_EQ(res, expected);
    }

    // Second case: extra data in the child->parent stream
    {
        corrupt_job_t corruptor(&pool);
        corruptor.corrupt(true);
    }

    for (size_t i = 0; i < 10; ++i) {
        fib_job_t job(10, &pool, NULL);
        uint64_t res = job.run();
        uint64_t expected = fib(10);
        ASSERT_EQ(res, expected);
    }
}

TEST(ExtProc, CorruptJob) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(run_corrupt_job_test));
}

void run_interrupt_job_by_user_test() {
    extproc_pool_t pool(2);
    cond_t interruptor;
    fib_job_t job(10, &pool, &interruptor);
    interruptor.pulse();
    ASSERT_THROW(job.run(), interrupted_exc_t);
}

TEST(ExtProc, InterruptJobByUser) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_interrupt_job_by_user_test));
}

void interrupt_job_by_pool_internal(object_buffer_t<extproc_pool_t> *pool,
                                    cond_t *done) {
    pool->reset();
    done->pulse();
}

// Destruct the pool while a job is still running, which is tricky
void run_interrupt_job_by_pool_test() {
    object_buffer_t<extproc_pool_t> pool;
    cond_t done;
    pool.create(2);

    {
        fib_job_t job(10, pool.get(), NULL);

        coro_t::spawn_now_dangerously(boost::bind(&interrupt_job_by_pool_internal,
                                                  &pool, &done));

        EXPECT_THROW(job.run(), interrupted_exc_t);
    }

    done.wait();
}

TEST(ExtProc, InterruptJobByPool) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_interrupt_job_by_pool_test));
}
