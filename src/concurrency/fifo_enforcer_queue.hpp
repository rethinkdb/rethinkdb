// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_FIFO_ENFORCER_QUEUE_HPP_
#define CONCURRENCY_FIFO_ENFORCER_QUEUE_HPP_

#include <map>
#include <utility>

#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/queue/passive_producer.hpp"

namespace unittest {
void run_queue_equivalence_test();
}

/* `fifo_enforcer_queue_t` consumes FIFO read and write tokens just like
`fifo_enforcer_sink_t` does, but it uses them to order items in a queue instead
of using them to block up coroutines. */

template <class T>
class fifo_enforcer_queue_t : public passive_producer_t<T>, public home_thread_mixin_debug_only_t {
public:
    fifo_enforcer_queue_t();
    fifo_enforcer_queue_t(perfmon_counter_t *_read_counter, perfmon_counter_t *_write_counter);
    ~fifo_enforcer_queue_t();

    void push(fifo_enforcer_read_token_t token, const T &t);
    void finish_read(fifo_enforcer_read_token_t read_token);
    void push(fifo_enforcer_write_token_t token, const T &t);
    void finish_write(fifo_enforcer_write_token_t write_token);

private:
    typedef std::multimap<state_timestamp_t, T> read_queue_t;
    read_queue_t read_queue;

    typedef std::map<transition_timestamp_t, std::pair<int64_t, T> > write_queue_t;
    write_queue_t write_queue;

    mutex_assertion_t lock;

    fifo_enforcer_state_t state;

    perfmon_counter_t *read_counter, *write_counter;

private:
friend void unittest::run_queue_equivalence_test();
    //passive produce api
    T produce_next_value();

    availability_control_t control;

    void consider_changing_available();

    DISABLE_COPYING(fifo_enforcer_queue_t);
};

#include "concurrency/fifo_enforcer_queue.tcc"

#endif /* CONCURRENCY_FIFO_ENFORCER_QUEUE_HPP_ */
