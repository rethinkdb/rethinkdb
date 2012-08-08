#include "concurrency/fifo_enforcer.hpp"

#include "concurrency/cond_var.hpp"
#include "concurrency/wait_any.hpp"

fifo_enforcer_read_token_t fifo_enforcer_source_t::enter_read() THROWS_NOTHING {
    assert_thread();
    mutex_assertion_t::acq_t freeze(&lock);
    ++state.num_reads;
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

fifo_enforcer_sink_t::exit_read_t::exit_read_t() THROWS_NOTHING : parent(NULL), ended(false) { }

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
    mutex_assertion_t::acq_t acq(&parent->lock);

    queue_position = parent->waiting_readers.insert(std::make_pair(token.timestamp, this));

    parent->pump();
}

void fifo_enforcer_sink_t::exit_read_t::end() THROWS_NOTHING {
    rassert(parent && !ended);
    parent->assert_thread();
    mutex_assertion_t::acq_t acq(&parent->lock);
    if (is_pulsed()) {
        parent->finish_a_reader(token.timestamp);
        parent->pump();
    } else {
        queue_position->second = NULL;
    }
    ended = true;
}

fifo_enforcer_sink_t::exit_read_t::~exit_read_t() THROWS_NOTHING {
    if (parent && !ended) {
        end();
    }
}

fifo_enforcer_sink_t::exit_write_t::exit_write_t() THROWS_NOTHING : parent(NULL), ended(false) { }

fifo_enforcer_sink_t::exit_write_t::exit_write_t(fifo_enforcer_sink_t *p, fifo_enforcer_write_token_t t) THROWS_NOTHING :
    parent(NULL), ended(false)
{
    begin(p, t);
}

void fifo_enforcer_sink_t::exit_write_t::begin(fifo_enforcer_sink_t *p, fifo_enforcer_write_token_t t) THROWS_NOTHING {
    rassert(parent == NULL);
    rassert(p != NULL);
    parent = p;
    token = t;

    parent->assert_thread();
    mutex_assertion_t::acq_t acq(&parent->lock);

    std::pair<writer_queue_t::iterator, bool> inserted = parent->waiting_writers.insert(std::make_pair(token.timestamp, std::make_pair(token.num_preceding_reads, this)));
    rassert(inserted.second, "duplicate fifo_enforcer_write_token_t");
    queue_position = inserted.first;

    parent->pump();
}

void fifo_enforcer_sink_t::exit_write_t::end() THROWS_NOTHING {
    rassert(parent && !ended);
    mutex_assertion_t::acq_t acq(&parent->lock);
    if (is_pulsed()) {
        parent->finish_a_writer(token.timestamp, token.num_preceding_reads);
        parent->pump();
    } else {
        queue_position->second.second = NULL;
    }
    ended = true;
}

fifo_enforcer_sink_t::exit_write_t::~exit_write_t() THROWS_NOTHING {
    if (parent && !ended) {
        end();
    }
}

void fifo_enforcer_sink_t::pump() THROWS_NOTHING {
    bool cont;
    do {
        cont = false;
        std::pair<reader_queue_t::iterator, reader_queue_t::iterator> bounds =
            waiting_readers.equal_range(state.timestamp);
        for (reader_queue_t::iterator it = bounds.first;
                it != bounds.second; ++it) {
            exit_read_t *read = it->second;
            if (read) {
                read->pulse();
            } else {
                cont = true;
                finish_a_reader(it->first);
            }
        }
        waiting_readers.erase(bounds.first, bounds.second);

        writer_queue_t::iterator it =
            waiting_writers.find(transition_timestamp_t::starting_from(state.timestamp));
        if (it != waiting_writers.end()) {
            int64_t expected_num_reads = it->second.first;
            rassert(state.num_reads <= expected_num_reads);

            if (state.num_reads == expected_num_reads) {
                exit_write_t *write = it->second.second;
                if (write) {
                    write->pulse();
                } else {
                    cont = true;
                    finish_a_writer(it->first, expected_num_reads);
                }
                waiting_writers.erase(it);
            }
        }
    } while (cont);
}

void fifo_enforcer_sink_t::finish_a_reader(DEBUG_ONLY_VAR state_timestamp_t timestamp) THROWS_NOTHING {
    assert_thread();

    rassert(state.timestamp == timestamp);

    ++state.num_reads;
}

void fifo_enforcer_sink_t::finish_a_writer(transition_timestamp_t timestamp, DEBUG_ONLY_VAR int64_t num_preceding_reads) THROWS_NOTHING {
    assert_thread();

    rassert(state.timestamp == timestamp.timestamp_before());
    rassert(state.num_reads == num_preceding_reads);

    state.timestamp = timestamp.timestamp_after();
    state.num_reads = 0;
}

