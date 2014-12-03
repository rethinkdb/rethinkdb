// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ARTIFICIAL_TABLE_BACKEND_CFEED_HPP_
#define RDB_PROTOCOL_ARTIFICIAL_TABLE_BACKEND_CFEED_HPP_

#include "concurrency/new_mutex.hpp"
#include "rdb_protocol/artificial_table/backend.hpp"
#include "rdb_protocol/changefeed.hpp"

/* `cfeed_artificial_table_backend_t` is a mixin for artificial table backends that want
to support change-feeds by storing a copy of all of the values in the table, and then
fetching values that have changed and comparing against the stored value. Subclasses must
call `notify_*` to tell the `cfeed_artificial_table_backend_t` which rows need to be
re-fetched. */

class cfeed_artificial_table_backend_t :
    public virtual artificial_table_backend_t {
public:
    cfeed_artificial_table_backend_t();

    bool read_changes(
        const ql::protob_t<const Backtrace> &bt,
        ql::changefeed::keyspec_t::spec_t &&spec,
        signal_t *interruptor,
        counted_t<ql::datum_stream_t> *cfeed_out,
        std::string *error_out);

protected:
    virtual ~cfeed_artificial_table_backend_t();

    /* The `cfeed_artificial_table_backend_t` calls `set_notifications()` to tell the
    subclass whether it needs notifications or not. The default is no; it will call
    `set_notifications(true)` when the first changefeed is connected, and
    `set_notifications(false)` if no changefeeds have used this artificial table for a
    while. It's OK for the subclass to call `notify_*` even if `set_notifications(false)`
    is called; this is just provided as an optimization for subclasses that would need to
    do expensive work to subscribe for notifications. `set_notifications()` is allowed to
    block. It will never be called from within `notify_*()`. */
    virtual void set_notifications(bool) { }

    /* `notify_*` will not block. `notify_row()` indicates that a particular row may have
    changed. `notify_all()` indicates that the entire table may have changed, including
    rows appearing or disappearing. `notify_break()` is like `notify_all()` except that
    it sends an error to any changefeeds instead of sending them all the changes. It's
    always safe to call these even if nothing has actually changed; if nothing has
    actually changed, no notification will be sent. */
    void notify_row(const ql::datum_t &pkey);
    void notify_all();
    void notify_break();

    /* Subclasses must call this when their destructor begins, to avoid the risk that the
    `cfeed_artificial_table_backend_t` will call virtual functions like `read_row()`
    after the subclass is destroyed. */
    void begin_changefeed_destruction();

private:
    class machinery_t : public ql::changefeed::artificial_t {
    public:
        machinery_t(cfeed_artificial_table_backend_t *parent);
        ~machinery_t();

        void maybe_remove();
        void run(auto_drainer_t::lock_t keepalive);
        bool diff_one(const ql::datum_t &key, signal_t *interruptor);
        bool diff_all(bool is_break, signal_t *interruptor);
        bool get_values(signal_t *interruptor, std::map<store_key_t, ql::datum_t> *out);
        void send_all_change(const store_key_t &key,
                             const ql::datum_t &old_val,
                             const ql::datum_t &new_val);
        void send_all_stop();

        cfeed_artificial_table_backend_t *parent;

        cond_t ready;

        /* `old_values` stores the last known value of every key. This is used to fill in
        the `old_val` field on changefeed changes. */
        std::map<store_key_t, ql::datum_t> old_values;

        /* `dirty` is the set of keys that have changed since `run()` last fetched their
        values. The key in the map is always equal to the result of calling
        `print_primary()` on the value. This is effectively a `std::set<ql::datum_t>`,
        but `ql::datum_t` doesn't support `operator<`, so we have this instead. */
        std::map<store_key_t, ql::datum_t> dirty;

        /* If `all_dirty` is `true`, then `dirty` should be empty, and all keys are
        considered to have changed. */
        bool all_dirty;
        
        /* `should_break` is only meaningful if `all_dirty` is `true`. It means that when
        we fetch new values for the keys in the table, instead of diffing against the old
        value and sending the diff to each subscriber, we should send an error message to
        each subscriber. */
        bool should_break;

        /* If `waker` is non-null, it should be pulsed whenever something becomes dirty.
        */
        cond_t *waker;

        /* If we don't have any subscribers, this is the time when the last subscriber
        disconnected. If we do have subscribers, this is meaningless. */
        microtime_t last_subscriber_time;

        auto_drainer_t drainer;
    };
    void maybe_remove_machinery();
    scoped_ptr_t<machinery_t> machinery;
    bool begin_destruction_was_called;
    new_mutex_t mutex;
    auto_drainer_t drainer;
    repeating_timer_t remove_machinery_timer;
};

/* `timer_cfeed_artificial_table_backend_t` implements changefeeds by simply setting a
repeating timer and retrieving the contents of the table every time the timer rings. It's
inefficient, but for many tables it's the most practical choice. */
class timer_cfeed_artificial_table_backend_t :
    public cfeed_artificial_table_backend_t {
private:
    void set_notifications(bool);
    void on_ring();
    scoped_ptr_t<repeating_timer_t> timer;
};

#endif /* RDB_PROTOCOL_ARTIFICIAL_TABLE_BACKEND_CFEED_HPP_ */

