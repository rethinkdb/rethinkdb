#include "concurrency/fifo_enforcer.hpp"

#include "concurrency/cond_var.hpp"
#include "concurrency/wait_any.hpp"

fifo_enforcer_read_token_t fifo_enforcer_source_t::enter_read() THROWS_NOTHING {
    assert_thread();
    mutex_assertion_t::acq_t freeze(&lock);
    num_reads++;
    return fifo_enforcer_read_token_t(timestamp);
}

fifo_enforcer_write_token_t fifo_enforcer_source_t::enter_write() THROWS_NOTHING {
    assert_thread();
    mutex_assertion_t::acq_t freeze(&lock);
    fifo_enforcer_write_token_t token(transition_timestamp_t::starting_from(timestamp), num_reads);
    timestamp = token.timestamp.timestamp_after();
    num_reads = 0;
    return token;
}

fifo_enforcer_sink_t::exit_read_t::exit_read_t() THROWS_NOTHING : parent(NULL) { }

fifo_enforcer_sink_t::exit_read_t::exit_read_t(fifo_enforcer_sink_t *p, fifo_enforcer_read_token_t t, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) :
    parent(NULL)
{
    reset(p, t, interruptor);
}

fifo_enforcer_sink_t::exit_read_t::~exit_read_t() THROWS_NOTHING {
    reset();
}

void fifo_enforcer_sink_t::exit_read_t::reset() THROWS_NOTHING {
    if (parent) {
        parent->assert_thread();
        mutex_assertion_t::acq_t acq(&parent->lock);
        rassert(parent->timestamp == token.timestamp);
        parent->num_reads++;
        parent->pump_writers();
        parent = NULL;
    }
}

void fifo_enforcer_sink_t::exit_read_t::reset(fifo_enforcer_sink_t *p, fifo_enforcer_read_token_t t, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    reset();
    p->assert_thread();
    mutex_assertion_t::acq_t acq(&p->lock);
    if (p->timestamp < t.timestamp) {
        cond_t can_proceed_cond;
        multimap_insertion_sentry_t<state_timestamp_t, cond_t *> insertion(
            &p->waiting_readers,
            t.timestamp,
            &can_proceed_cond);
        wait_any_t waiter(&can_proceed_cond, interruptor);
        acq.reset();
        waiter.wait_lazily_unordered();
        acq.reset(&p->lock);
        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }
    }
    parent = p;
    token = t;
    rassert(parent->timestamp == token.timestamp);
}

fifo_enforcer_sink_t::exit_write_t::exit_write_t() THROWS_NOTHING : parent(NULL) { }

fifo_enforcer_sink_t::exit_write_t::exit_write_t(fifo_enforcer_sink_t *p, fifo_enforcer_write_token_t t, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) :
    parent(NULL)
{
    reset(p, t, interruptor);
}

fifo_enforcer_sink_t::exit_write_t::~exit_write_t() THROWS_NOTHING {
    reset();
}

void fifo_enforcer_sink_t::exit_write_t::reset() THROWS_NOTHING {
    if (parent) {
        parent->assert_thread();
        mutex_assertion_t::acq_t acq(&parent->lock);
        rassert(parent->timestamp == token.timestamp.timestamp_before());
        rassert(parent->num_reads == token.num_preceding_reads);
        parent->timestamp = token.timestamp.timestamp_after();
        parent->num_reads = 0;
        parent->pump_readers();
        parent->pump_writers();
        parent = NULL;
    }
}

void fifo_enforcer_sink_t::exit_write_t::reset(fifo_enforcer_sink_t *p, fifo_enforcer_write_token_t t, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    reset();
    p->assert_thread();
    mutex_assertion_t::acq_t acq(&p->lock);
    if (p->timestamp < t.timestamp.timestamp_before() ||
            p->num_reads < t.num_preceding_reads) {
        cond_t can_proceed_cond;
        map_insertion_sentry_t<transition_timestamp_t, std::pair<int, cond_t *> > insertion(
            &p->waiting_writers,
            t.timestamp,
            std::make_pair(t.num_preceding_reads, &can_proceed_cond));
        wait_any_t waiter(&can_proceed_cond, interruptor);
        acq.reset();
        waiter.wait_lazily_unordered();
        acq.reset(&p->lock);
        if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }
    }
    parent = p;
    token = t;
    rassert(parent->timestamp == token.timestamp.timestamp_before());
    rassert(parent->num_reads == token.num_preceding_reads);
}

void fifo_enforcer_sink_t::pump_readers() THROWS_NOTHING {
    std::pair<
            std::multimap<state_timestamp_t, cond_t *>::iterator,
            std::multimap<state_timestamp_t, cond_t *>::iterator
            > bounds = waiting_readers.equal_range(timestamp);
    for (std::multimap<state_timestamp_t, cond_t *>::iterator it = bounds.first;
            it != bounds.second; it++) {
        (*it).second->pulse();
    }
}

void fifo_enforcer_sink_t::pump_writers() THROWS_NOTHING {
    std::map<transition_timestamp_t, std::pair<int, cond_t *> >::iterator it =
        waiting_writers.find(transition_timestamp_t::starting_from(timestamp));
    if (it != waiting_writers.end()) {
        rassert(num_reads <= (*it).second.first);
        if (num_reads == (*it).second.first) {
            (*it).second.second->pulse();
        }
    }
}
