// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_FIFO_ENFORCER_HPP_
#define CONCURRENCY_FIFO_ENFORCER_HPP_

#include <map>
#include <utility>

#include "concurrency/mutex_assertion.hpp"
#include "concurrency/signal.hpp"
#include "containers/intrusive_priority_queue.hpp"
#include "perfmon/perfmon.hpp"
#include "rpc/serialize_macros.hpp"
#include "stl_utils.hpp"
#include "timestamps.hpp"
#include "utils.hpp"

class cond_t;
class signal_t;

namespace unittest {
void run_queue_equivalence_test();
}

/* `fifo_enforcer.hpp` contains facilities for enforcing that objects pass
through a checkpoint in the same order that they passed through a previous
checkpoint. The objects in transit between the checkpoints are identified by
"tokens", of the types `fifo_enforcer_read_token_t` and
`fifo_enforcer_write_token_t`. Read tokens are allowed to be reordered relative
to each other but not relative to write tokens. */

/* A clinic can be used as metaphor for how `fifo_enforcer_{source,sink}_t` are used.
 * First you get an appointment (`fifo_enforcer_source_t::enter_{read,write}()`), then you
 * come to a waiting room (construct `fifo_enforcer_sink_t::exit_{read,write}_t`), where
 * you are notified when the doctor is ready to see you (the `exit_{read,write}_t` object
 * gets pulsed). When you finish seeing the doctor (destroy the `exit_{read,write}_t`),
 * the person who has the next appointment is notified.  If you leave the waiting room
 * (destroy the `exit_{read,write}_t` before it is pulsed), you forfeit your appointment
 * and place in line.
 *
 * The metaphor breaks down in that, if you don't show up for your appointment, all the
 * later appointments are delayed indefinitely. That means that if you never construct a
 * `exit_{read,write}_t` for a token, users of all later tokens will never get to go.
 */
class fifo_enforcer_read_token_t {
public:
    fifo_enforcer_read_token_t() THROWS_NOTHING { }
    explicit fifo_enforcer_read_token_t(state_timestamp_t t) THROWS_NOTHING :
        timestamp(t) { }
    state_timestamp_t timestamp;
private:
    RDB_MAKE_ME_SERIALIZABLE_1(timestamp);
};

class fifo_enforcer_write_token_t {
public:
    fifo_enforcer_write_token_t() THROWS_NOTHING : timestamp(), num_preceding_reads(-1) { }
    fifo_enforcer_write_token_t(transition_timestamp_t t, int npr) THROWS_NOTHING :
        timestamp(t), num_preceding_reads(npr) { }
    transition_timestamp_t timestamp;
    int64_t num_preceding_reads;
private:
    RDB_MAKE_ME_SERIALIZABLE_2(timestamp, num_preceding_reads);
};

class fifo_enforcer_state_t {
public:
    fifo_enforcer_state_t() THROWS_NOTHING :
        timestamp(valgrind_undefined(state_timestamp_t::zero())),
        num_reads(valgrind_undefined<int64_t>(0)) { }
    fifo_enforcer_state_t(state_timestamp_t ts, int64_t nr) THROWS_NOTHING :
        timestamp(ts), num_reads(nr) { }

    void advance_by_read(fifo_enforcer_read_token_t tok) THROWS_NOTHING;
    void advance_by_write(fifo_enforcer_write_token_t tok) THROWS_NOTHING;

    state_timestamp_t timestamp;
    int64_t num_reads;
private:
    RDB_MAKE_ME_SERIALIZABLE_2(timestamp, num_reads);
};

class fifo_enforcer_source_t : public home_thread_mixin_debug_only_t {
public:
    fifo_enforcer_source_t() THROWS_NOTHING :
        state(state_timestamp_t::zero(), 0) { }

    /* Enters the FIFO for read. Does not block. */
    fifo_enforcer_read_token_t enter_read() THROWS_NOTHING;

    /* Enters the FIFO for write. Does not block. */
    fifo_enforcer_write_token_t enter_write() THROWS_NOTHING;

    fifo_enforcer_state_t get_state() THROWS_NOTHING {
        return state;
    }

private:
    mutex_assertion_t lock;
    fifo_enforcer_state_t state;
    DISABLE_COPYING(fifo_enforcer_source_t);
};

class fifo_enforcer_sink_t : public home_thread_mixin_debug_only_t {
public:
    /* `internal_exit_{read,write}_t` represents any thing that waits in line to
    go through the FIFO enforcer. It's responsible for adding itself to the FIFO
    enforcer queue and for removing itself. It's public so that people can
    create more subclasses to do interesting things. You should probably use one
    of its subclasses. */

    class internal_exit_read_t : public intrusive_priority_queue_node_t<internal_exit_read_t> {
    protected:
        virtual ~internal_exit_read_t() { }

    private:
        friend class fifo_enforcer_sink_t;
        friend bool left_is_higher_priority(const fifo_enforcer_sink_t::internal_exit_read_t *left,
                                            const fifo_enforcer_sink_t::internal_exit_read_t *right);

        /* Returns the read token for this operation. It will be used to sort
        the operation in the queue. */
        virtual fifo_enforcer_read_token_t get_token() const = 0;

        /* Called when the operation has reached the head of the queue. It
        should remove the operation from the queue. */
        virtual void on_reached_head_of_queue() = 0;

        /* Called when the FIFO enforcer is being destroyed. The operation will
        already have been removed from the queue. */
        virtual void on_early_shutdown() = 0;
    };

    class internal_exit_write_t : public intrusive_priority_queue_node_t<internal_exit_write_t> {
    protected:
        virtual ~internal_exit_write_t() { }

    private:
        friend class fifo_enforcer_sink_t;
        friend bool left_is_higher_priority(const fifo_enforcer_sink_t::internal_exit_write_t *left,
                                            const fifo_enforcer_sink_t::internal_exit_write_t *right);

        /* Returns the write token for this operation. It will be used to sort
        the operation in the queue. */
        virtual fifo_enforcer_write_token_t get_token() const = 0;

        /* Called when the operation has reached the head of the queue. It
        should remove the operation from the queue. (Or change its token, which
        is used to efficiently implement an object that stands for more than one
        token.) */
        virtual void on_reached_head_of_queue() = 0;

        /* Called when the FIFO enforcer is being destroyed. The operation will
        already have been removed from the queue. */
        virtual void on_early_shutdown() = 0;
    };

    /* `exit_{read,write}_t` notes that the given FIFO enforcer token has
    entered the queue, and pulses itself when the token has reached the head of
    the queue. When the `exit_{read,write}_t` is destroyed, it allows future
    operations to proceed. If it hadn't reached the head of the queue by the
    time it was destroyed, operations that come later are still allowed to
    proceed, even though the destructor doesn't block. It accomplishes this by
    putting a dummy entry into the queue. */

    class exit_read_t : public signal_t, public internal_exit_read_t {
    public:
        exit_read_t() THROWS_NOTHING;

        /* Calls `begin()` */
        exit_read_t(fifo_enforcer_sink_t *, fifo_enforcer_read_token_t) THROWS_NOTHING;

        /* Calls `end()` if appropriate */
        ~exit_read_t() THROWS_NOTHING;

        void begin(fifo_enforcer_sink_t *, fifo_enforcer_read_token_t) THROWS_NOTHING;
        void end() THROWS_NOTHING;

    private:
        fifo_enforcer_read_token_t get_token() const {
            return token;
        }
        void on_reached_head_of_queue() {
            parent->internal_read_queue.remove(this);
            pulse();
        }
        void on_early_shutdown() {
            crash("illegal to destroy fifo_enforcer_sink_t while outstanding "
                "exit_read_t objects exist");
        }

        /* `parent` is `NULL` before `begin()` is called. After `end()` is
        called, `parent` remains non-`NULL` but `ended` is set to `true` */
        fifo_enforcer_sink_t *parent;
        bool ended;

        fifo_enforcer_read_token_t token;
    };

    class exit_write_t : public signal_t, public internal_exit_write_t {
    public:
        exit_write_t() THROWS_NOTHING;

        /* Calls `begin()` */
        exit_write_t(fifo_enforcer_sink_t *, fifo_enforcer_write_token_t) THROWS_NOTHING;

        /* Calls `end()` if appropriate */
        ~exit_write_t() THROWS_NOTHING;

        void begin(fifo_enforcer_sink_t *, fifo_enforcer_write_token_t) THROWS_NOTHING;
        void end() THROWS_NOTHING;

    private:
        fifo_enforcer_write_token_t get_token() const {
            return token;
        }
        void on_reached_head_of_queue() {
            parent->internal_write_queue.remove(this);
            pulse();
        }
        void on_early_shutdown() {
            crash("illegal to destroy fifo_enforcer_sink_t while outstanding "
                "exit_write_t objects exist");
        }

        /* `parent` is `NULL` before `begin()` is called. After `end()` is
        called, `parent` remains non-`NULL` but `ended` is set to `true` */
        fifo_enforcer_sink_t *parent;
        bool ended;

        fifo_enforcer_write_token_t token;
    };

    fifo_enforcer_sink_t() THROWS_NOTHING :
        popped_state(state_timestamp_t::zero(), 0),
        finished_state(state_timestamp_t::zero(), 0),
        in_pump(false)
        { }

    explicit fifo_enforcer_sink_t(fifo_enforcer_state_t init) THROWS_NOTHING :
        popped_state(init),
        finished_state(init),
        in_pump(false)
        { }

    ~fifo_enforcer_sink_t() THROWS_NOTHING;

    /* Don't access this stuff directly unless you know what you're doing. It's
    public so that new subclasses of `internal_exit_{read,write}_t` can access
    it. */

    void internal_pump() THROWS_NOTHING;
    void internal_finish_a_reader(fifo_enforcer_read_token_t token) THROWS_NOTHING;
    void internal_finish_a_writer(fifo_enforcer_write_token_t token) THROWS_NOTHING;

    mutex_assertion_t internal_lock;
    intrusive_priority_queue_t<internal_exit_read_t> internal_read_queue;
    intrusive_priority_queue_t<internal_exit_write_t> internal_write_queue;

private:
    /* The difference between `popped_state` and `finished_state` is the
    operations that are currently running. They have been popped off the queue,
    but they have not finished. */
    fifo_enforcer_state_t popped_state, finished_state;

    /* If `internal_pump()` is called recursively, the inner call will set
    `pump_should_keep_going` to `true` rather than doing its normal work. That
    way, we use a loop rather than arbitrarily deep recursion. */
    bool in_pump, pump_should_keep_going;

    DISABLE_COPYING(fifo_enforcer_sink_t);
};


inline bool left_is_higher_priority(const fifo_enforcer_sink_t::internal_exit_read_t *left,
                                    const fifo_enforcer_sink_t::internal_exit_read_t *right) {
    return left->get_token().timestamp < right->get_token().timestamp;
}

inline bool left_is_higher_priority(const fifo_enforcer_sink_t::internal_exit_write_t *left,
                                    const fifo_enforcer_sink_t::internal_exit_write_t *right) {
    return left->get_token().timestamp < right->get_token().timestamp;
}


#endif /* CONCURRENCY_FIFO_ENFORCER_HPP_ */
