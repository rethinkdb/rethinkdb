#ifndef __CONCURRENCY_FIFO_ENFORCER_QUEUE_TCC__
#define __CONCURRENCY_FIFO_ENFORCER_QUEUE_TCC__

#include "concurrency/fifo_enforcer.hpp"

template <class T>
fifo_enforcer_queue_t<T>::fifo_enforcer_queue_t()
    : passive_producer_t<T>(&control), state(state_timestamp_t::zero(), 0),
      read_counter(NULL), write_counter(NULL)
{ }

template <class T>
fifo_enforcer_queue_t<T>::fifo_enforcer_queue_t(perfmon_counter_t *_read_counter, perfmon_counter_t *_write_counter)
    : passive_producer_t<T>(&control), state(state_timestamp_t::zero(), 0),
      read_counter(_read_counter), write_counter(_write_counter)
{ }

template <class T>
fifo_enforcer_queue_t<T>::~fifo_enforcer_queue_t() { }

template <class T>
void fifo_enforcer_queue_t<T>::push(fifo_enforcer_read_token_t token, const T &t) {
    if (read_counter) { ++(*read_counter); }

    assert_thread();
    mutex_assertion_t::acq_t acq(&lock);

    read_queue.insert(std::make_pair(token.timestamp, t));
    consider_changing_available();
}

template <class T>
void fifo_enforcer_queue_t<T>::finish_read(DEBUG_ONLY_VAR fifo_enforcer_read_token_t read_token) {
    if (read_counter) { --(*read_counter); }

    assert_thread();

    rassert(state.timestamp == read_token.timestamp);

    state.num_reads++;
    consider_changing_available();
}

template <class T>
void fifo_enforcer_queue_t<T>::push(fifo_enforcer_write_token_t token, const T &t) {
    if (write_counter) { ++(*write_counter); }

    assert_thread();
    mutex_assertion_t::acq_t acq(&lock);

    write_queue.insert(std::make_pair(token.timestamp, std::make_pair(token.num_preceding_reads, t)));
    consider_changing_available();
}

template <class T>
void fifo_enforcer_queue_t<T>::finish_write(fifo_enforcer_write_token_t write_token) {
    if (write_counter) { --(*write_counter); }

    assert_thread();

    rassert(state.timestamp == write_token.timestamp.timestamp_before());
    rassert(state.num_reads == write_token.num_preceding_reads);

    state.timestamp = write_token.timestamp.timestamp_after();
    state.num_reads = 0;
    consider_changing_available();
}

template <class T>
T fifo_enforcer_queue_t<T>::produce_next_value() {
    typename read_queue_t::iterator rit = read_queue.find(state.timestamp);
    if (rit != read_queue.end()) {
        T res = rit->second;
        read_queue.erase(rit);
        consider_changing_available();
        return res;
    }

    typename write_queue_t::iterator wit =
        write_queue.find(transition_timestamp_t::starting_from(state.timestamp));
    if (wit != write_queue.end()) {
        int64_t expected_num_reads = wit->second.first;
        rassert(state.num_reads <= expected_num_reads);

        if (state.num_reads == expected_num_reads) {
            T res = wit->second.second;
            write_queue.erase(wit);
            consider_changing_available();
            return res;
        }
    }
    //We're in this function when we don't actually have a value to return
    rassert(!control.get(), "Control return true but we don't actually have a value to return. This class is incorrectly implemented.");
    crash("Unreachable\n");
    return T();
}

template <class T>
void fifo_enforcer_queue_t<T>::consider_changing_available() {
    if (std_contains(read_queue, state.timestamp)) {
        control.set_available(true);
    } else if (std_contains(write_queue, transition_timestamp_t::starting_from(state.timestamp)) &&
            write_queue.find(transition_timestamp_t::starting_from(state.timestamp))->second.first == state.num_reads) {
        control.set_available(true);
    } else {
        control.set_available(false);
    }
}

#endif
