// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/artificial_table/backend_cfeed.hpp"

#include "concurrency/cross_thread_signal.hpp"
#include "rdb_protocol/env.hpp"

bool cfeed_artificial_table_backend_t::read_changes(
        const ql::protob_t<const Backtrace> &bt,
        const ql::changefeed::keyspec_t::spec_t &spec,
        signal_t *interruptor,
        counted_t<ql::datum_stream_t> *cfeed_out,
        std::string *error_out) {
    if (boost::get<ql::changefeed::keyspec_t::limit_t>(&spec) != nullptr) {
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
    *cfeed_out = machinery->subscribe(spec, bt);
    return true;
}

void cfeed_artificial_table_backend_t::notify_row(const ql::datum_t &pkey) {
    if (machinery.has()) {
        if (!machinery->all_dirty) {
            store_key_t pkey2(pkey.print_primary());
            machinery->dirty.insert(std::make_pair(pkey2, pkey));
            if (machinery->waker) {
                machinery->waker->pulse_if_not_already_pulsed();
            }
        }
    }
}

void cfeed_artificial_table_backend_t::notify_all() {
    if (machinery.has()) {
        if (!machinery->all_dirty) {
            /* This is in an `if` block so we don't unset `should_break` it
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
    if (machinery.has()) {
        machinery->all_dirty = true;
        machinery->dirty.clear();
        machinery->should_break = true;
        if (machinery->waker) {
            machinery->waker->pulse_if_not_already_pulsed();
        }
    }
}

cfeed_artificial_table_backend_t::machinery_t::machinery_t(
        cfeed_artificial_table_backend_t *_parent) :
    parent(_parent),
    /* Initialize `all_dirty` to `true` so that we'll fetch the initial values */
    all_dirty(true),
    should_break(false),
    waker(nullptr)
{
    parent->set_notifications(true);
    coro_t::spawn_sometime(std::bind(
        &cfeed_artificial_table_backend_t::machinery_t::run,
        this,
        drainer.lock()));
}

cfeed_artificial_table_backend_t::machinery_t::~machinery_t() {
    guarantee(can_be_removed());
    parent->set_notifications(false);
}

void cfeed_artificial_table_backend_t::machinery_t::maybe_remove() {
    coro_t::spawn_sometime(std::bind(
        &cfeed_artificial_table_backend_t::maybe_remove_machinery,
        parent,
        parent->drainer.lock()));
}

void cfeed_artificial_table_backend_t::machinery_t::run(
        auto_drainer_t::lock_t keepalive) {
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

            if (local_all_dirty) {
                guarantee(local_dirty.empty());
                /* Fetch the new values of everything */
                std::map<store_key_t, ql::datum_t> new_values;
                if (!get_values(keepalive.get_drain_signal(), &new_values)) {
                    /* Kick off any subscribers since we got an error. */
                    send_all_stop();
                    /* Ensure that we try again the next time around the loop. */
                    parent->notify_all();
                    /* Go around the loop and try again. But first wait a second so we
                    don't eat too much CPU if it keeps failing. */
                    nap(1000, keepalive.get_drain_signal());
                    continue;
                }
                if (!ready.is_pulsed()) {
                    /* This is the initialization step. We can't possibly have any
                    subscribers yet, so don't bother with the diff. */
                    ready.pulse();
                } else if (local_should_break) {
                    /* Don't bother with the diff; just kick off all the subscribers */
                    send_all_stop();
                } else {
                    /* Diff old values against new values and notify subscribers */
                    send_all_diff(old_values, new_values);
                }
                old_values = std::move(new_values);
            } else {
                guarantee(!local_dirty.empty());
                for (const auto &pair : local_dirty) {
                    /* Fetch new value from backend */
                    ql::datum_t new_val;
                    std::string error;
                    if (!parent->read_row(
                            pair.second,
                            keepalive.get_drain_signal(),
                            &new_val,
                            &error)) {
                        /* Kick off any subscribers since we got an error. */
                        send_all_stop();
                        /* Ensure that we try again the next time around the loop. */
                        parent->notify_row(pair.second);
                        /* Go around the loop and try again. But first wait a second so
                        we don't eat too much CPU if it keeps failing. */
                        nap(1000, keepalive.get_drain_signal());
                        continue;
                    }
                    /* Fetch old value from `old_values` map */
                    ql::datum_t old_val;
                    auto it = old_values.find(pair.first);
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
                            old_values.insert(std::make_pair(pair.first, new_val));
                        }
                    }
                    /* Send notification if it actually changed */
                    if (new_val != old_val) {
                        send_all_change(pair.first, old_val, new_val);
                    }
                }
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

