// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/artificial_table/backend_cfeed.hpp"

#include "concurrency/cross_thread_signal.hpp"
#include "rdb_protocol/env.hpp"

bool cfeed_artificial_table_backend_t::read_changes(
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
        machinery.init(new machinery_t(this));
        wait_interruptible(&machinery->ready, &interruptor2);
    }
    on_thread_t thread_switcher_2(request_thread);
    *cfeed_out = machinery->subscribe(std::move(spec), bt);
    return true;
}

cfeed_artificial_table_backend_t::~cfeed_artificial_table_backend_t() {
    /* Catch subclasses that forget to call `begin_changefeed_destruction()` */
    guarantee(begin_destruction_was_called);
}

void cfeed_artificial_table_backend_t::notify_row(const ql::datum_t &pkey) {
    ASSERT_FINITE_CORO_WAITING;
    if (machinery.has()) {
        if (!machinery->all_dirty) {
            machinery->dirty.insert(std::make_pair(
                store_key_t(pkey.print_primary()),
                pkey));
            if (machinery->waker) {
                machinery->waker->pulse_if_not_already_pulsed();
            }
        }
    }
}

void cfeed_artificial_table_backend_t::notify_all() {
    ASSERT_FINITE_CORO_WAITING;
    if (machinery.has()) {
        if (!machinery->all_dirty) {
            /* This is in an `if` block so we don't unset `should_break` if
            `notify_break()` set it to `true` before. */
            machinery->should_break = false;
        }
        machinery->all_dirty = true;
        machinery->dirty.clear();
        if (machinery->waker) {
            machinery->waker->pulse_if_not_already_pulsed();
        }
    }
}

void cfeed_artificial_table_backend_t::notify_break() {
    ASSERT_FINITE_CORO_WAITING;
    if (machinery.has()) {
        machinery->all_dirty = true;
        machinery->dirty.clear();
        machinery->should_break = true;
        if (machinery->waker) {
            machinery->waker->pulse_if_not_already_pulsed();
        }
    }
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

cfeed_artificial_table_backend_t::machinery_t::machinery_t(
        cfeed_artificial_table_backend_t *_parent) :
    parent(_parent),
    /* Initialize `all_dirty` to `true` so that we'll fetch the initial values */
    all_dirty(true),
    should_break(false),
    waker(nullptr)
{
    coro_t::spawn_sometime(std::bind(
        &cfeed_artificial_table_backend_t::machinery_t::run,
        this,
        drainer.lock()));
}

cfeed_artificial_table_backend_t::machinery_t::~machinery_t() {
    guarantee(can_be_removed());
}

void cfeed_artificial_table_backend_t::machinery_t::maybe_remove() {
    coro_t::spawn_sometime(std::bind(
        &cfeed_artificial_table_backend_t::maybe_remove_machinery,
        parent,
        parent->drainer.lock()));
}

void cfeed_artificial_table_backend_t::machinery_t::run(
        auto_drainer_t::lock_t keepalive) {
    parent->set_notifications(true);
    try {
        while (true) {
            /* Copy the dirtiness flags into local variables and reset them. Resetting
            them now is important because it means that notifications that arrive while
            we're processing the current batch will be queued up instead of ignored. */
            std::map<store_key_t, ql::datum_t> local_dirty;
            bool local_all_dirty = false, local_should_break = false;
            std::swap(dirty, local_dirty);
            std::swap(all_dirty, local_all_dirty);
            std::swap(should_break, local_should_break);

            guarantee(local_all_dirty || !local_dirty.empty(),
                "If nothing is dirty, we shouldn't have gotten here");

            bool error = false;
            if (local_all_dirty) {
                guarantee(local_dirty.empty(), "if local_all_dirty is true, local_dirty "
                    "should be empty (that's just the convention)");
                if (!diff_all(should_break, keepalive.get_drain_signal())) {
                    error = true;
                } else {
                    ready.pulse_if_not_already_pulsed();
                }
            } else {
                for (const auto &pair : local_dirty) {
                    if (!diff_one(pair.second, keepalive.get_drain_signal())) {
                        error = true;
                        break;
                    }
                }
            }

            if (error) {
                /* Kick off any subscribers since we got an error. */
                send_all_stop();
                /* Ensure that we try again the next time around the loop. */
                parent->notify_all();
                /* Go around the loop and try again. But first wait a second so we
                don't eat too much CPU if it keeps failing. */
                nap(1000, keepalive.get_drain_signal());
                continue;
            }

            /* Wait until the next change */
            if (dirty.empty() && !all_dirty) {
                cond_t cond;
                assignment_sentry_t<cond_t *> cond_sentry(&waker, &cond);
                wait_interruptible(&cond, keepalive.get_drain_signal());
            }
        }
    } catch (const interrupted_exc_t &) {
        /* break out of the loop */
    }
    parent->set_notifications(false);
}

bool cfeed_artificial_table_backend_t::machinery_t::diff_one(
        const ql::datum_t &key, signal_t *interruptor) {
    /* Fetch new value from backend */
    ql::datum_t new_val;
    std::string error;
    if (!parent->read_row(key, interruptor, &new_val, &error)) {
        return false;
    }
    /* Fetch old value from `old_values` map */
    ql::datum_t old_val;
    auto it = old_values.find(store_key_t(key.print_primary()));
    if (it == old_values.end()) {
        old_val = ql::datum_t::null();
    } else {
        old_val = it->second;
    }
    /* Update `old_values` map */
    if (!new_val.has()) {
        new_val = ql::datum_t::null();
        if (it != old_values.end()) {
            old_values.erase(it);
        }
    } else {
        if (it != old_values.end()) {
            it->second = new_val;
        } else {
            old_values.insert(std::make_pair(store_key_t(key.print_primary()), new_val));
        }
    }
    /* Send notification if it actually changed */
    if (new_val != old_val) {
        send_all_change(store_key_t(key.print_primary()), old_val, new_val);
    }
    return true;
}

bool cfeed_artificial_table_backend_t::machinery_t::diff_all(
        bool is_break, signal_t *interruptor) {
    /* Fetch the new values of everything */
    std::map<store_key_t, ql::datum_t> new_values;
    if (!get_values(interruptor, &new_values)) {
        return false;
    }
    if (is_break) {
        /* Don't bother with the diff; just kick off all the subscribers */
        send_all_stop();
    } else {
        /* Diff old values against new values and notify subscribers */
        send_all_diff(old_values, new_values);
    }
    old_values = std::move(new_values);
    return true;
}

bool cfeed_artificial_table_backend_t::machinery_t::get_values(
        signal_t *interruptor, std::map<store_key_t, ql::datum_t> *out) {
    out->clear();
    std::string error;
    counted_t<ql::datum_stream_t> stream;
    if (!parent->read_all_rows_as_stream(
            ql::protob_t<const Backtrace>(),
            ql::datum_range_t::universe(),
            sorting_t::UNORDERED,
            interruptor,
            &stream,
            &error)) {
        return false;
    }
    guarantee(!stream->is_cfeed());
    ql::env_t env(interruptor, reql_version_t::LATEST);
    while (!stream->is_exhausted()) {
        std::vector<ql::datum_t> datums;
        try {
            datums = stream->next_batch(&env, ql::batchspec_t::all());
        } catch (const ql::exc_t &) {
            return false;
        }
        for (const ql::datum_t &doc : datums) {
            ql::datum_t key = doc.get_field(
                datum_string_t(parent->get_primary_key_name()),
                ql::NOTHROW);
            guarantee(key.has());
            store_key_t key2(key.print_primary());
            auto pair = out->insert(std::make_pair(key2, doc));
            guarantee(pair.second);
        }
    }
    return true;
}

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

void cfeed_artificial_table_backend_t::machinery_t::send_all_diff(
        const std::map<store_key_t, ql::datum_t> &old_vals,
        const std::map<store_key_t, ql::datum_t> &new_vals) {
    auto old_it = old_vals.begin();
    auto new_it = new_vals.begin();
    while (old_it != old_vals.end() || new_it != new_vals.end()) {
        if (old_it == old_vals.end() ||
                (new_it != new_vals.end() && new_it->first < old_it->first)) {
            send_all_change(new_it->first, ql::datum_t::null(), new_it->second);
            ++new_it;
        } else if (new_it == new_vals.end() || 
                (old_it != old_vals.end() && old_it->first < new_it->first)) {
            send_all_change(old_it->first, old_it->second, ql::datum_t::null());
            ++old_it;
        } else {
            guarantee(old_it != old_vals.end());
            guarantee(new_it != new_vals.end());
            guarantee(old_it->first == new_it->first);
            if (old_it->second != new_it->second) {
                send_all_change(old_it->first, old_it->second, new_it->second);
            }
            ++old_it;
            ++new_it;
        }
    }
}

void cfeed_artificial_table_backend_t::maybe_remove_machinery(
        auto_drainer_t::lock_t keepalive) {
    try {
        assert_thread();
        /* Wait 60 seconds between when the last changefeed closes and when we destroy
        the `machinery_t`, so we don't have to re-create the `machinery_t` if the user
        starts another changefeed in that time. */
        nap(60 * 1000, keepalive.get_drain_signal());
        new_mutex_in_line_t mutex_lock(&mutex);
        wait_interruptible(mutex_lock.acq_signal(), keepalive.get_drain_signal());
        if (machinery.has() && machinery->can_be_removed()) {
            /* Make sure that we unset `machinery` before we start deleting the
            underlying `machinery_t`, because calls to `notify_*` may otherwise try to
            access the `machinery_t` while it is being deleted */
            scoped_ptr_t<machinery_t> temp;
            std::swap(temp, machinery);
        }
    } catch (const interrupted_exc_t &) {
        /* We're shutting down. Take no special action. */
    }
}

void timer_cfeed_artificial_table_backend_t::set_notifications(bool notify) {
    if (notify && !timer.has()) {
        timer.init(new repeating_timer_t(1000, this));
    }
    if (!notify && timer.has()) {
        timer.reset();
    }
}

void timer_cfeed_artificial_table_backend_t::on_ring() {
    notify_all();
}

