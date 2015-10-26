// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ARTIFICIAL_TABLE_CFEED_BACKEND_HPP_
#define RDB_PROTOCOL_ARTIFICIAL_TABLE_CFEED_BACKEND_HPP_

#include "arch/timing.hpp"
#include "concurrency/new_mutex.hpp"
#include "rdb_protocol/artificial_table/backend.hpp"
#include "rdb_protocol/changefeed.hpp"

/* `cfeed_artificial_table_backend_t` is a mixin for artificial table backends that want
to support changefeeds. Because it's often expensive to generate change notifications,
backends will want to only start generating notifications when a client requests a
changefeed, and stop when no changefeeds have existed for a while. So there is some
amount of "machinery" for changefeeds that must be created when the first client connects
and then later destroyed. `cfeed_artificial_table_backend_t` contains the logic for
creating and destroying that "machinery" in a generic way. Subclasses should define their
own subclass of `cfeed_artificial_table_backend_t::machinery_t` that subscribes to
notifications and calls `send_all_change()` and `send_all_stop()` as necessary. */

class cfeed_artificial_table_backend_t :
    public artificial_table_backend_t {
public:
    bool read_changes(
        ql::env_t *env,
        bool include_initial_vals,
        bool include_states,
        ql::configured_limits_t limits,
        ql::backtrace_id_t bt,
        ql::changefeed::keyspec_t::spec_t &&spec,
        signal_t *interruptor,
        counted_t<ql::datum_stream_t> *cfeed_out,
        admin_err_t *error_out);

protected:
    class machinery_t : private ql::changefeed::artificial_t {
    public:
        machinery_t() : last_subscriber_time(current_microtime()) { }
        virtual ~machinery_t() { }

    protected:
        /* Subclasses can use these to send notifications */
        void send_all_change(
            const new_mutex_acq_t *proof,
            const store_key_t &key,
            const ql::datum_t &old_val,
            const ql::datum_t &new_val);

        void send_all_stop();

        /* `get_initial_values()` should return the current state of the entire table and
        also ensure that the state it returns is not squashed. In order to ensure that
        the state is not squashed, it may generate calls to `send_all_change()`. It
        returns `false` if something goes wrong. */
        virtual bool get_initial_values(
            const new_mutex_acq_t *proof,
            std::vector<ql::datum_t> *initial_values_out,
            signal_t *interruptor) = 0;

        new_mutex_t mutex;

    private:
        friend class cfeed_artificial_table_backend_t;
        /* `ql::changefeed::artificial_t` calls this when the last subscriber
        disconnects. */
        void maybe_remove();
        /* If we don't have any subscribers, this is the time the last one disconnected.
        */
        microtime_t last_subscriber_time;
    };

    cfeed_artificial_table_backend_t();
    virtual ~cfeed_artificial_table_backend_t();

    /* `cfeed_artificial_table_backend_t` guarantees that it will never have two sets of
    machinery in existence at the same time. */

    /* Subclasses should override this to return their own subclass of `machinery_t`. */
    virtual scoped_ptr_t<machinery_t> construct_changefeed_machinery(
        signal_t *interruptor) = 0;

    /* The subclass must call this in its destructor. It ensures that the changefeed
    machinery is destroyed. It may block. The purpose of this is because most subclasses
    will in practice need the machinery to be destroyed before their destructor ends,
    because the machinery will probably access subclass-specific member variables. */
    void begin_changefeed_destruction();

private:
    void maybe_remove_machinery();
    scoped_ptr_t<machinery_t> machinery;
    new_mutex_t mutex;
    bool begin_destruction_was_called;
    auto_drainer_t drainer;
    repeating_timer_t remove_machinery_timer;
};

#endif /* RDB_PROTOCOL_ARTIFICIAL_TABLE_CFEED_BACKEND_HPP_ */

