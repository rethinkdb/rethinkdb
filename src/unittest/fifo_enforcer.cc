#include "concurrency/fifo_enforcer.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/variant.hpp>

#include "arch/timing.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/fifo_enforcer_queue.hpp"
#include "concurrency/wait_any.hpp"
#include "mock/unittest_utils.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

class test_shared_variable_t {
public:
    explicit test_shared_variable_t(int initial) : variable(initial) { }
    void spawn_read(int expected_value) {
        coro_t::spawn_sometime(boost::bind(
            &test_shared_variable_t::attempt_read, this,
            expected_value,
            source.enter_read(),
            auto_drainer_t::lock_t(&auto_drainer)
            ));
    }
    void spawn_write(int expected_value, int new_value) {
        coro_t::spawn_sometime(boost::bind(
            &test_shared_variable_t::attempt_write, this,
            expected_value, new_value,
            source.enter_write(),
            auto_drainer_t::lock_t(&auto_drainer)
            ));
    }
private:
    void attempt_read(int expected_value, fifo_enforcer_read_token_t token, auto_drainer_t::lock_t) {
        if (rng.randint(2) == 0) {
            nap(rng.randint(10));
        }
        fifo_enforcer_sink_t::exit_read_t exit(&sink, token);
        exit.wait_lazily_unordered();
        EXPECT_EQ(expected_value, variable);
    }
    void attempt_write(int expected_value, int new_value, fifo_enforcer_write_token_t token, auto_drainer_t::lock_t) {
        if (rng.randint(2) == 0) {
            nap(rng.randint(10));
        }
        fifo_enforcer_sink_t::exit_write_t exit(&sink, token);
        exit.wait_lazily_unordered();
        EXPECT_EQ(expected_value, variable);
        variable = new_value;
    }
    fifo_enforcer_source_t source;
    fifo_enforcer_sink_t sink;
    int variable;
    rng_t rng;
    auto_drainer_t auto_drainer;
};

void run_fifo_enforcer_test() {
    rng_t rng;
    int var1 = rng.randint(10000);
    test_shared_variable_t var2(var1);
    for (int i = 0; i < 1000; i++) {
        if (rng.randint(4) == 0) {
            int new_value = rng.randint(10000);
            var2.spawn_write(var1, new_value);
            var1 = new_value;
        } else {
            var2.spawn_read(var1);
        }
    }
}

TEST(FIFOEnforcer, FIFOEnforcer) {
    mock::run_in_thread_pool(&run_fifo_enforcer_test);
}

void run_state_transfer_test() {
    fifo_enforcer_source_t source;
    UNUSED fifo_enforcer_write_token_t tok = source.enter_write();
    fifo_enforcer_sink_t sink(source.get_state());
    fifo_enforcer_write_token_t tok2 = source.enter_write();
    {
        try {
            signal_timer_t interruptor(1000);
            fifo_enforcer_sink_t::exit_write_t fifo_exit(&sink, tok2);
            wait_interruptible(&fifo_exit, &interruptor);
        } catch (interrupted_exc_t) {
            ADD_FAILURE() << "Got stuck trying to exit FIFO";
        }
    }
}

TEST(FIFOEnforcer, StateTransfer) {
    mock::run_in_thread_pool(&run_state_transfer_test);
}

/* This tests that dummy entries to fifo enforcers, that is entries which we
 * construct and then destroy (as we are doing with source.new_write_token()
 * and exit_write_t, don't trip up the fifo_enforce's destructors. */
void run_dummy_entry_destruction_test() {
    fifo_enforcer_source_t source;
    fifo_enforcer_sink_t sink;

    source.enter_write();

    fifo_enforcer_sink_t::exit_write_t(&sink, source.enter_write());
}

TEST(FIFOEnforcer, DummyEntry) {
    mock::run_in_thread_pool(&run_dummy_entry_destruction_test);
}

void run_queue_equivalence_test() {
    fifo_enforcer_source_t source;
    fifo_enforcer_sink_t sink;
    fifo_enforcer_queue_t<boost::variant<fifo_enforcer_read_token_t, fifo_enforcer_write_token_t> > queue;

    int total_inserted = 0;
    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 50; j++) {
            fifo_enforcer_read_token_t t = source.enter_read();
            queue.push(t, t);
            total_inserted++;
        }
        fifo_enforcer_write_token_t t = source.enter_write();
        queue.push(t, t);
        total_inserted++;
    }
    while  (queue.control.get()) {
        boost::variant<fifo_enforcer_read_token_t, fifo_enforcer_write_token_t>  token = queue.produce_next_value();
        if (fifo_enforcer_read_token_t *read_token = boost::get<fifo_enforcer_read_token_t>(&token)) {
            fifo_enforcer_sink_t::exit_read_t er(&sink, *read_token);
            EXPECT_TRUE(er.is_pulsed());
            queue.finish_read(*read_token);
        } else if (fifo_enforcer_write_token_t *write_token = boost::get<fifo_enforcer_write_token_t>(&token)) {
            fifo_enforcer_sink_t::exit_write_t ew(&sink, *write_token);
            EXPECT_TRUE(ew.is_pulsed());
            queue.finish_write(*write_token);
        }
        total_inserted--;
    }

    EXPECT_EQ(total_inserted, 0);
}

TEST(FIFOEnforcer, QueueEquivalence) {
    mock::run_in_thread_pool(&run_queue_equivalence_test);
}

}   /* namespace unittest */

