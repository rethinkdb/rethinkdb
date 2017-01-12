// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/artificial_table/cfeed_backend.hpp"

#include "clustering/administration/admin_op_exc.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "rdb_protocol/env.hpp"

/* We destroy the machinery if there have been no changefeeds for this many seconds */
static const int machinery_expiration_secs = 60;

void cfeed_artificial_table_backend_t::machinery_t::send_all_change(
        const new_mutex_acq_t *proof,
        const store_key_t &pkey,
        const ql::datum_t &old_val,
        const ql::datum_t &new_val) {
    proof->guarantee_is_holding(&mutex);
    send_all(
        ql::changefeed::msg_t(
            ql::changefeed::msg_t::change_t{
                index_vals_t(),
                index_vals_t(),
                pkey,
                old_val,
                new_val}));
}

void cfeed_artificial_table_backend_t::machinery_t::send_all_stop() {
    send_all(ql::changefeed::msg_t(ql::changefeed::msg_t::stop_t()));
}

void cfeed_artificial_table_backend_t::machinery_t::maybe_remove() {
    assert_thread();
    last_subscriber_time = get_kiloticks();
    /* The `cfeed_artificial_table_backend_t` has a repeating timer that will eventually
    clean us up */
}

cfeed_artificial_table_backend_t::cfeed_artificial_table_backend_t(
        name_string_t const &table_name,
        rdb_context_t *rdb_context,
        lifetime_t<name_resolver_t const &> name_resolver)
    : artificial_table_backend_t(table_name, rdb_context),
      m_name_resolver(name_resolver),
      begin_destruction_was_called(false),
      remove_machinery_timer(
        machinery_expiration_secs * THOUSAND,
        [this]() { maybe_remove_machinery(); }) {
}

cfeed_artificial_table_backend_t::~cfeed_artificial_table_backend_t() {
    /* Catch subclasses that forget to call `begin_changefeed_destruction()`. This isn't
    strictly necessary (some hypothetical subclasses might not have any reason to call
    it) but it's far more likely that the subclass should have called it but forgot to.
    */
    guarantee(begin_destruction_was_called);
}

bool cfeed_artificial_table_backend_t::read_changes(
    ql::env_t *env,
    const ql::changefeed::streamspec_t &ss,
    ql::backtrace_id_t bt,
    signal_t *interruptor,
    counted_t<ql::datum_stream_t> *cfeed_out,
    admin_err_t *error_out) {

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
        bool operator()(const ql::changefeed::keyspec_t::empty_t &) const {
            return true;
        }
    };
    if (!boost::apply_visitor(visitor_t(), ss.spec)) {
        *error_out = admin_err_t{
            "System tables don't support changefeeds on `.limit()`.",
            query_state_t::FAILED};
        return false;
    }
    threadnum_t request_thread = get_thread_id();
    cross_thread_signal_t interruptor2(interruptor, home_thread());
    on_thread_t thread_switcher(home_thread());
    new_mutex_acq_t mutex_lock(&mutex, &interruptor2);

    auth::user_context_t user_context = env->get_user_context();
    auto &machinery = machineries[user_context];
    if (!machinery.has()) {
        machinery = construct_changefeed_machinery(
            make_lifetime(m_name_resolver), user_context, &interruptor2);
    }

    new_mutex_acq_t machinery_lock(&machinery->mutex, &interruptor2);
    std::vector<ql::datum_t> initial_values;
    if (!machinery->get_initial_values(
            &machinery_lock, &initial_values, &interruptor2)) {
        *error_out = admin_err_t{
            "Failed to read initial values from system table.",
            query_state_t::FAILED};
        return false;
    }

    /* We construct two `on_thread_t`s for a total of four thread switches. This is
    necessary because we have to call `subscribe()` on the client thread, but we don't
    want to release the lock on the home thread until `subscribe()` returns. */
    on_thread_t thread_switcher_2(request_thread);
    try {
        *cfeed_out = machinery->subscribe(
            env,
            ss,
            get_primary_key_name(),
            initial_values,
            bt);
    } catch (const ql::base_exc_t &e) {
        *error_out = admin_err_t{e.what(), query_state_t::FAILED};
        return false;
    }
    return true;
}

void cfeed_artificial_table_backend_t::begin_changefeed_destruction() {
    new_mutex_in_line_t mutex_lock(&mutex);
    mutex_lock.acq_signal()->wait_lazily_unordered();
    guarantee(!begin_destruction_was_called);
    for (auto &machinery : machineries) {
        if (machinery.second.has()) {
            /* All changefeeds should be closed before we start destruction */
            guarantee(machinery.second->can_be_removed());

            /* We unset `machinery.second` before destructing it because there may be a
            coroutine from `maybe_remove_machinery` in flight to destruct it as well. */
            scoped_ptr_t<machinery_t> temp;
            std::swap(temp, machinery.second);
        }
    }
    begin_destruction_was_called = true;
}

void cfeed_artificial_table_backend_t::maybe_remove_machinery() {
    /* This is called periodically by a repeating timer */
    assert_thread();

    for (auto &machinery : machineries) {
        if (machinery.second.has() &&
                machinery.second->can_be_removed() &&
                machinery.second->last_subscriber_time.micros +
                    machinery_expiration_secs * MILLION < get_kiloticks().micros) {
            auth::user_context_t user_context = machinery.first;
            auto_drainer_t::lock_t keepalive(&drainer);
            coro_t::spawn_sometime([this, user_context, keepalive /* important to capture */]() {
                try {
                    new_mutex_in_line_t mutex_lock(&mutex);
                    wait_interruptible(mutex_lock.acq_signal(),
                                       keepalive.get_drain_signal());
                    auto &machinery_inner = machineries[user_context];
                    if (machinery_inner.has() &&
                            machinery_inner->can_be_removed()) {
                        /* Make sure that we unset `machinery_inner` before destructing
                        the underlying `machinery_t` because calls to `notify_*` and
                        `begin_changefeed_destruction` may otherwise try to access the
                        `macheringy_t` while it's being destructed. Secondly, we don't
                        remove the element to prevent invalid iterators. */
                        scoped_ptr_t<machinery_t> temp;
                        std::swap(temp, machinery_inner);
                    }
                } catch (const interrupted_exc_t &) {
                    /* We're shutting down. The machinery will get cleaned up anyway */
                }
            });
        }
    }
}

