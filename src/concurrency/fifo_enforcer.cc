#include "concurrency/fifo_enforcer.hpp"

#include "concurrency/cond_var.hpp"
#include "concurrency/wait_any.hpp"

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
    parent(NULL), ended(false), is_dummy(false) { }

fifo_enforcer_sink_t::exit_read_t::exit_read_t(fifo_enforcer_sink_t *p, fifo_enforcer_read_token_t t) THROWS_NOTHING :
    parent(NULL), ended(false), is_dummy(false)
{
    begin(p, t);
}

void fifo_enforcer_sink_t::exit_read_t::begin(fifo_enforcer_sink_t *p, fifo_enforcer_read_token_t t) THROWS_NOTHING {
    rassert(parent == NULL);
    rassert(p != NULL);
    rassert(!is_dummy);
    parent = p;
    token = t;

    parent->assert_thread();
    mutex_assertion_t::acq_t acq(&parent->lock);
    parent->waiting_readers.push(this);
    parent->pump();
}

void fifo_enforcer_sink_t::exit_read_t::end() THROWS_NOTHING {
    rassert(parent && !ended);
    rassert(!is_dummy);
    parent->assert_thread();
    mutex_assertion_t::acq_t acq(&parent->lock);
    if (is_pulsed()) {
        parent->finish_a_reader(token);
        parent->pump();
    } else {
        /* Swap us out for a dummy. The dummy is heap-allocated and the
        `fifo_enforcer_sink_t` will delete it when appropriate. */
        parent->waiting_readers.swap_in_place(this, new exit_read_t(token));
    }
    ended = true;
}

fifo_enforcer_sink_t::exit_read_t::~exit_read_t() THROWS_NOTHING {
    if (!is_dummy && parent && !ended) {
        end();
    }
}

fifo_enforcer_sink_t::exit_write_t::exit_write_t() THROWS_NOTHING :
    parent(NULL), ended(false), is_dummy(false) { }

fifo_enforcer_sink_t::exit_write_t::exit_write_t(fifo_enforcer_sink_t *p, fifo_enforcer_write_token_t t) THROWS_NOTHING :
    parent(NULL), ended(false), is_dummy(false)
{
    begin(p, t);
}

void fifo_enforcer_sink_t::exit_write_t::begin(fifo_enforcer_sink_t *p, fifo_enforcer_write_token_t t) THROWS_NOTHING {
    rassert(parent == NULL);
    rassert(p != NULL);
    rassert(!is_dummy);
    parent = p;
    token = t;

    parent->assert_thread();
    mutex_assertion_t::acq_t acq(&parent->lock);
    parent->waiting_writers.push(this);
    parent->pump();
}

void fifo_enforcer_sink_t::exit_write_t::end() THROWS_NOTHING {
    rassert(parent && !ended);
    rassert(!is_dummy);
    mutex_assertion_t::acq_t acq(&parent->lock);
    if (is_pulsed()) {
        parent->finish_a_writer(token);
        parent->pump();
    } else {
        /* Swap us out for a dummy. */
        parent->waiting_writers.swap_in_place(this, new exit_write_t(token));
    }
    ended = true;
}

fifo_enforcer_sink_t::exit_write_t::~exit_write_t() THROWS_NOTHING {
    if (!is_dummy && parent && !ended) {
        end();
    }
}

fifo_enforcer_sink_t::~fifo_enforcer_sink_t() THROWS_NOTHING {
    while (!waiting_readers.empty()) {
        exit_read_t *dummy_read = waiting_readers.pop();
        rassert(dummy_read->is_dummy);
        delete dummy_read;
    }
    while (!waiting_writers.empty()) {
        exit_write_t *dummy_write = waiting_writers.pop();
        rassert(dummy_write->is_dummy);
        delete dummy_write;
    }
}

void fifo_enforcer_sink_t::pump() THROWS_NOTHING {
    bool cont;
    do {
        cont = false;

        while (!waiting_readers.empty() &&
                waiting_readers.peek()->token.timestamp == state.timestamp) {
            exit_read_t *read = waiting_readers.pop();
            if (read->is_dummy) {
                finish_a_reader(read->token);
                delete read;
                cont = true;
            } else {
                read->pulse();
            }
        }

        if (!waiting_writers.empty() &&
                waiting_writers.peek()->token.timestamp.timestamp_before() == state.timestamp &&
                waiting_writers.peek()->token.num_preceding_reads == state.num_reads) {
            exit_write_t *write = waiting_writers.pop();
            if (write->is_dummy) {
                finish_a_writer(write->token);
                delete write;
                cont = true;
            } else {
                write->pulse();
            }
        }

    } while (cont);
}

void fifo_enforcer_sink_t::finish_a_reader(DEBUG_ONLY_VAR fifo_enforcer_read_token_t token) THROWS_NOTHING {
    assert_thread();

    rassert(state.timestamp == token.timestamp);

    state.num_reads++;
}

void fifo_enforcer_sink_t::finish_a_writer(fifo_enforcer_write_token_t token) THROWS_NOTHING {
    assert_thread();

    rassert(state.timestamp == token.timestamp.timestamp_before());
    rassert(state.num_reads == token.num_preceding_reads);

    state.timestamp = token.timestamp.timestamp_after();
    state.num_reads = 0;
}

