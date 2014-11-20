// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ARTIFICIAL_TABLE_BACKEND_CFEED_HPP_
#define RDB_PROTOCOL_ARTIFICIAL_TABLE_BACKEND_CFEED_HPP_

#include "concurrency/new_mutex.hpp"
#include "rdb_protocol/artificial_table/backend.hpp"
#include "rdb_protocol/changefeed.hpp"

class cfeed_artificial_table_backend_t : public artificial_table_backend_t {
public:
    bool read_changes(
        const ql::protob_t<const Backtrace> &bt,
        const ql::changefeed::keyspec_t::spec_t &spec,
        signal_t *interruptor,
        counted_t<ql::datum_stream_t> *cfeed_out,
        std::string *error_out);

protected:
    virtual ~cfeed_artificial_table_backend_t() { }

    virtual void set_notifications(bool) { }

    void notify_row(const ql::datum_t &pkey);
    void notify_all();
    void notify_break();

private:
    class machinery_t : public ql::changefeed::artificial_t {
    public:
        machinery_t(cfeed_artificial_table_backend_t *parent);
        ~machinery_t();

        void maybe_remove();
        void run(auto_drainer_t::lock_t keepalive);
        bool get_values(signal_t *interruptor, std::map<store_key_t, ql::datum_t> *out);
        void send_all_change(const store_key_t &key,
                             const ql::datum_t &old_val,
                             const ql::datum_t &new_val);
        void send_all_stop();
        void send_all_diff(const std::map<store_key_t, ql::datum_t> &old_vals,
                           const std::map<store_key_t, ql::datum_t> &new_vals);

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

        auto_drainer_t drainer;
    };
    void maybe_remove_machinery(auto_drainer_t::lock_t keepalive);
    scoped_ptr_t<machinery_t> machinery;
    new_mutex_t mutex;
    auto_drainer_t drainer;
};

#endif /* RDB_PROTOCOL_ARTIFICIAL_TABLE_BACKEND_CFEED_HPP_ */

