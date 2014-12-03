// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ARTIFICIAL_TABLE_CACHING_CFEED_BACKEND_HPP_
#define RDB_PROTOCOL_ARTIFICIAL_TABLE_CACHING_CFEED_BACKEND_HPP_

#include "rdb_protocol/artificial_table/cfeed_backend.hpp"

/* `caching_cfeed_artificial_table_backend_t` is a mixin for artificial table backends
that implements change-feeds by storing a copy of all the rows in the table. It relies on
its subclass to notify it when a row has changed; then it fetches the new value of the
row, compares it to the old value, and sends a notification if necessary. */

class caching_cfeed_artificial_table_backend_t :
    public cfeed_artificial_table_backend_t {
protected:
    /* The `caching_cfeed_artificial_table_backend_t` calls `set_notifications()` to tell
    the subclass whether it needs notifications or not. The default is no; it will call
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
    actually changed, no notification will be sent to the client. */
    void notify_row(const ql::datum_t &pkey);
    void notify_all();
    void notify_break();

    /* Subclasses must remember to call `begin_changefeed_destruction()` in their
    destructors. */

private:
    class caching_machinery_t : public cfeed_artificial_table_backend_t::machinery_t {
    public:
        caching_machinery_t(caching_cfeed_artificial_table_backend_t *parent);
        ~caching_machinery_t();
        void run(auto_drainer_t::lock_t keepalive) THROWS_NOTHING;
        bool diff_one(const ql::datum_t &key, signal_t *interruptor);
        bool diff_all(bool is_break, signal_t *interruptor);
        bool get_values(signal_t *interruptor, std::map<store_key_t, ql::datum_t> *out);

        caching_cfeed_artificial_table_backend_t *parent;

        /* Pulsed when all of the initial values have been fetched. */
        cond_t ready;

        /* `old_values` stores the last known value of every key. This is used to fill in
        the `old_val` field on changefeed changes. */
        std::map<store_key_t, ql::datum_t> old_values;

        /* `dirty` is the set of keys that have changed since `run()` last fetched their
        values. The key in the map is always equal to the result of calling
        `print_primary()` on the value. This is effectively a `std::set<ql::datum_t>`,
        but `ql::datum_t` doesn't support `operator<`, so we have this instead. */
        std::map<store_key_t, ql::datum_t> dirty;

        /* If `all_dirty` or `all_dirty_break` is `true`, then all keys are considered to
        have changed. The difference is that `all_dirty_break` will stop all changefeeds
        instead of sending them the changes. Either or both can be true. */
        bool all_dirty, all_dirty_break;

        /* If `waker` is non-null, it should be pulsed whenever something becomes dirty.
        */
        cond_t *waker;

        auto_drainer_t drainer;
    };

    scoped_ptr_t<cfeed_artificial_table_backend_t::machinery_t>
        construct_changefeed_machinery(signal_t *interruptor);

    caching_machinery_t *caching_machinery;
};

/* `timer_cfeed_artificial_table_backend_t` implements changefeeds by simply setting a
repeating timer and retrieving the contents of the table every time the timer rings. It's
inefficient, but for many tables it's the most practical choice. */
class timer_cfeed_artificial_table_backend_t :
    public caching_cfeed_artificial_table_backend_t {
private:
    void set_notifications(bool);
    scoped_ptr_t<repeating_timer_t> timer;
};

#endif /* RDB_PROTOCOL_ARTIFICIAL_TABLE_CACHING_CFEED_BACKEND_HPP_ */

