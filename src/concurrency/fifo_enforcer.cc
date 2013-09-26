// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "concurrency/fifo_enforcer.hpp"

#include "arch/runtime/coroutines.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/wait_any.hpp"

void fifo_enforcer_state_t::advance_by_read(DEBUG_VAR fifo_enforcer_read_token_t token) THROWS_NOTHING {
    rassert(timestamp == token.timestamp);
    num_reads++;
}

void fifo_enforcer_state_t::advance_by_write(fifo_enforcer_write_token_t token) THROWS_NOTHING {
    rassert(timestamp == token.timestamp.timestamp_before());
    rassert(num_reads == token.num_preceding_reads);
    timestamp = token.timestamp.timestamp_after();
    num_reads = 0;
}

fifo_enforcer_read_token_t fifo_enforcer_source_t::enter_read() THROWS_NOTHING {
    assert_thread();
    mutex_assertion_t::acq_t freeze(&lock);
    state.num_reads++;
    return fifo_enforcer_read_token_t(state.timestamp);
}

fifo_enforcer_write_token_t fifo_enforcer_source_t::enter_write() THROWS_NOTHING {
    assert_thread();
    mutex_assertion_t::acq_t freeze(&lock);
    fifo_enforcer_write_token_t token(transition_timestamp_t::starting_from(state.timestamp), state.num_reads);
    state.timestamp = token.timestamp.timestamp_after();
    state.num_reads = 0;
    return token;
}

fifo_enforcer_sink_t::exit_read_t::exit_read_t() THROWS_NOTHING :
    parent(NULL), ended(false) { }

fifo_enforcer_sink_t::exit_read_t::exit_read_t(fifo_enforcer_sink_t *p, fifo_enforcer_read_token_t t) THROWS_NOTHING :
    parent(NULL), ended(false)
{
    begin(p, t);
}

void fifo_enforcer_sink_t::exit_read_t::begin(fifo_enforcer_sink_t *p, fifo_enforcer_read_token_t t) THROWS_NOTHING {
    rassert(parent == NULL);
    rassert(p != NULL);
    parent = p;
    token = t;

    parent->assert_thread();
    mutex_assertion_t::acq_t acq(&parent->internal_lock);
    parent->internal_read_queue.push(this);
    parent->internal_pump();
}

void fifo_enforcer_sink_t::exit_read_t::end() THROWS_NOTHING {
    rassert(parent && !ended);
    parent->assert_thread();
    mutex_assertion_t::acq_t acq(&parent->internal_lock);
    if (is_pulsed()) {
        parent->internal_finish_a_reader(token);
    } else {
        /* Swap us out for a dummy. The dummy is heap-allocated and it will
        delete itself when it's done. */
        class dummy_exit_read_t : public internal_exit_read_t {
        public:
            dummy_exit_read_t(fifo_enforcer_read_token_t t, fifo_enforcer_sink_t *s) :
                token(t), sink(s) { }
            fifo_enforcer_read_token_t get_token() const {
                return token;
            }
            void on_reached_head_of_queue() {
                sink->internal_read_queue.remove(this);
                sink->internal_finish_a_reader(token);
                delete this;
            }
            void on_early_shutdown() {
                delete this;
            }
            fifo_enforcer_read_token_t token;
            fifo_enforcer_sink_t *sink;
        };
        parent->internal_read_queue.swap_in_place(this,
            new dummy_exit_read_t(token, parent));
    }
    ended = true;
}

fifo_enforcer_sink_t::exit_read_t::~exit_read_t() THROWS_NOTHING {
    if (parent && !ended) {
        end();
    }
}

fifo_enforcer_sink_t::exit_write_t::exit_write_t() THROWS_NOTHING :
    parent(NULL), ended(false) { }

fifo_enforcer_sink_t::exit_write_t::exit_write_t(fifo_enforcer_sink_t *p, fifo_enforcer_write_token_t t) THROWS_NOTHING :
    parent(NULL), ended(false)
{
    begin(p, t);
}

void fifo_enforcer_sink_t::exit_write_t::begin(fifo_enforcer_sink_t *p, fifo_enforcer_write_token_t t) THROWS_NOTHING {
    ASSERT_FINITE_CORO_WAITING;

    rassert(parent == NULL);
    rassert(p != NULL);
    parent = p;
    token = t;

    parent->assert_thread();
    mutex_assertion_t::acq_t acq(&parent->internal_lock);
    parent->internal_write_queue.push(this);
    parent->internal_pump();
}

void fifo_enforcer_sink_t::exit_write_t::end() THROWS_NOTHING {
    rassert(parent && !ended);
    mutex_assertion_t::acq_t acq(&parent->internal_lock);
    if (is_pulsed()) {
        parent->internal_finish_a_writer(token);
    } else {
        /* Swap us out for a dummy. */
        class dummy_exit_write_t : public internal_exit_write_t {
        public:
            dummy_exit_write_t(fifo_enforcer_write_token_t t, fifo_enforcer_sink_t *s) :
                token(t), sink(s) { }
            fifo_enforcer_write_token_t get_token() const {
                return token;
            }
            void on_reached_head_of_queue() {
                sink->internal_write_queue.remove(this);
                sink->internal_finish_a_writer(token);
                delete this;
            }
            void on_early_shutdown() {
                delete this;
            }
            fifo_enforcer_write_token_t token;
            fifo_enforcer_sink_t *sink;
        };
        parent->internal_write_queue.swap_in_place(this,
            new dummy_exit_write_t(token, parent));
    }
    ended = true;
}

fifo_enforcer_sink_t::exit_write_t::~exit_write_t() THROWS_NOTHING {
    if (parent && !ended) {
        end();
    }
}

fifo_enforcer_sink_t::~fifo_enforcer_sink_t() THROWS_NOTHING {
    while (!internal_read_queue.empty()) {
        internal_exit_read_t *read = internal_read_queue.pop();
        read->on_early_shutdown();
    }
    while (!internal_write_queue.empty()) {
        internal_exit_write_t *write = internal_write_queue.pop();
        write->on_early_shutdown();
    }
}

void fifo_enforcer_sink_t::internal_pump() THROWS_NOTHING {
    ASSERT_FINITE_CORO_WAITING;
    if (in_pump) {
        pump_should_keep_going = true;
    } else {
        in_pump = true;
        do {
            pump_should_keep_going = false;

            while (!internal_read_queue.empty() &&
                    internal_read_queue.peek()->get_token().timestamp == finished_state.timestamp) {
                internal_exit_read_t *read = internal_read_queue.peek();
                popped_state.advance_by_read(read->get_token());
                read->on_reached_head_of_queue();
            }

            if (!internal_write_queue.empty() &&
                    internal_write_queue.peek()->get_token().timestamp.timestamp_before() == finished_state.timestamp &&
                    internal_write_queue.peek()->get_token().num_preceding_reads == finished_state.num_reads) {
                internal_exit_write_t *write = internal_write_queue.peek();
                popped_state.advance_by_write(write->get_token());
                write->on_reached_head_of_queue();
            }
        } while (pump_should_keep_going);
        in_pump = false;
    }
}

void fifo_enforcer_sink_t::internal_finish_a_reader(fifo_enforcer_read_token_t token) THROWS_NOTHING {
    rassert(popped_state.timestamp == token.timestamp);
    rassert(finished_state.num_reads <= popped_state.num_reads);
    finished_state.advance_by_read(token);
    internal_pump();
}

void fifo_enforcer_sink_t::internal_finish_a_writer(fifo_enforcer_write_token_t token) THROWS_NOTHING {
    rassert(popped_state.timestamp == token.timestamp.timestamp_after());
    rassert(popped_state.num_reads == 0);
    finished_state.advance_by_write(token);
    internal_pump();
}
