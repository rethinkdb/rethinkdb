// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/artificial_table/cfeed_backend.hpp"

#include "concurrency/cross_thread_signal.hpp"

/* We destroy the machinery if there have been no changefeeds for this many seconds */
static const int machinery_expiration_secs = 60;

void cfeed_artificial_table_backend_t::machinery_t::send_all_change(
        const store_key_t &key,
        const ql::datum_t &old_val,
        const ql::datum_t &new_val) {
    ql::changefeed::msg_t::change_t change;
    change.pkey = key;
    change.old_val = old_val;
    change.new_val = new_val;
    send_all(ql::changefeed::msg_t(change));
}

void cfeed_artificial_table_backend_t::machinery_t::send_all_stop() {
    send_all(ql::changefeed::msg_t(ql::changefeed::msg_t::stop_t()));
}

void cfeed_artificial_table_backend_t::machinery_t::maybe_remove() {
    assert_thread();
    last_subscriber_time = current_microtime();
    /* The `cfeed_artificial_table_backend_t` has a repeating timer that will eventually
    clean us up */
}

cfeed_artificial_table_backend_t::cfeed_artificial_table_backend_t() :
    begin_destruction_was_called(false),
    remove_machinery_timer(
        machinery_expiration_secs * THOUSAND,
        [this]() { maybe_remove_machinery(); })
    { }

cfeed_artificial_table_backend_t::~cfeed_artificial_table_backend_t() {
    /* Catch subclasses that forget to call `begin_changefeed_destruction()`. This isn't
    strictly necessary (some hypothetical subclasses might not have any reason to call
    it) but it's far more likely that the subclass should have called it but forgot to.
    */
    guarantee(begin_destruction_was_called);
}

bool cfeed_artificial_table_backend_t::read_changes(
    ql::env_t *env,
    const ql::protob_t<const Backtrace> &bt,
    ql::changefeed::keyspec_t::spec_t &&spec,
    signal_t *interruptor,
    counted_t<ql::datum_stream_t> *cfeed_out,
    std::string *error_out) {

    guarantee(!begin_destruction_was_called);
    class visitor_t : public boost::static_visitor<bool> {
    public:
        bool operator()(const ql::changefeed::keyspec_t::limit_t &) const {
            return false;
        }
        bool operator()(const ql::changefeed::keyspec_t::range_t &) const {
            return true;
        }
        bool operator()(const ql::changefeed::keyspec_t::point_t &) const {
            return true;
        }
    };
    if (!boost::apply_visitor(visitor_t(), spec)) {
        *error_out = "System tables don't support changefeeds on `.limit()`.";
        return false;
    }
    threadnum_t request_thread = get_thread_id();
    cross_thread_signal_t interruptor2(interruptor, home_thread());
    on_thread_t thread_switcher(home_thread());
    new_mutex_in_line_t mutex_lock(&mutex);
    wait_interruptible(mutex_lock.acq_signal(), &interruptor2);
    if (!machinery.has()) {
        machinery = construct_changefeed_machinery(&interruptor2);
    }
    /* We construct two `on_thread_t`s for a total of four thread switches. This is
    necessary because we have to call `subscribe()` on the client thread, but we don't
    want to release the lock on the home thread until `subscribe()` returns. */
    on_thread_t thread_switcher_2(request_thread);
    *cfeed_out = machinery->subscribe(env, std::move(spec), bt);
    return true;
}

void cfeed_artificial_table_backend_t::begin_changefeed_destruction() {
    new_mutex_in_line_t mutex_lock(&mutex);
    mutex_lock.acq_signal()->wait_lazily_unordered();
    guarantee(!begin_destruction_was_called);
    if (machinery.has()) {
        /* All changefeeds should be closed before we start destruction */
        guarantee(machinery->can_be_removed());
        machinery.reset();
    }
    begin_destruction_was_called = true;
}

void cfeed_artificial_table_backend_t::maybe_remove_machinery() {
    /* This is called periodically by a repeating timer */
    assert_thread();
    if (machinery.has() && machinery->can_be_removed() &&
            machinery->last_subscriber_time + machinery_expiration_secs * MILLION <
                current_microtime()) {
        auto_drainer_t::lock_t keepalive(&drainer);
        coro_t::spawn_sometime([this, keepalive   /* important to capture */]() {
            try {
                new_mutex_in_line_t mutex_lock(&mutex);
                wait_interruptible(mutex_lock.acq_signal(),
                                   keepalive.get_drain_signal());
                if (machinery.has() && machinery->can_be_removed()) {
                    /* Make sure that we unset `machinery` before we start deleting the
                    underlying `machinery_t`, because calls to `notify_*` may otherwise
                    try to access the `machinery_t` while it is being deleted */
                    scoped_ptr_t<machinery_t> temp;
                    std::swap(temp, machinery);
                }
            } catch (const interrupted_exc_t &) {
                /* We're shutting down. The machinery will get cleaned up anyway */
            }
        });
    }
}

