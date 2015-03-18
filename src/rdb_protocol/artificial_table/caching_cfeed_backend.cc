// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/artificial_table/caching_cfeed_backend.hpp"

#include "rdb_protocol/env.hpp"

caching_cfeed_artificial_table_backend_t::caching_cfeed_artificial_table_backend_t() :
    caching_machinery(nullptr) { }

void caching_cfeed_artificial_table_backend_t::notify_row(const ql::datum_t &pkey) {
    ASSERT_FINITE_CORO_WAITING;
    if (caching_machinery != nullptr) {
        /* If we're going to reload all the rows anyway, don't bother storing this
        individual row */
        if (caching_machinery->dirtiness !=
                caching_machinery_t::dirtiness_t::none_or_some) {
            return;
        }
        caching_machinery->dirty_keys.insert(pkey);
        if (caching_machinery->waker != nullptr) {
            caching_machinery->waker->pulse_if_not_already_pulsed();
        }
    }
}

void caching_cfeed_artificial_table_backend_t::notify_all() {
    ASSERT_FINITE_CORO_WAITING;
    if (caching_machinery != nullptr) {
        /* If the previous value was `all_stop`, then leave it the way it is instead of
        changing it to `all`. */
        if (caching_machinery->dirtiness ==
                caching_machinery_t::dirtiness_t::none_or_some) {
            caching_machinery->dirtiness = caching_machinery_t::dirtiness_t::all;
        }
        if (caching_machinery->waker) {
            caching_machinery->waker->pulse_if_not_already_pulsed();
        }
    }
}

void caching_cfeed_artificial_table_backend_t::notify_break() {
    ASSERT_FINITE_CORO_WAITING;
    if (caching_machinery != nullptr) {
        caching_machinery->dirtiness = caching_machinery_t::dirtiness_t::all_stop;
        if (caching_machinery->waker) {
            caching_machinery->waker->pulse_if_not_already_pulsed();
        }
    }
}

caching_cfeed_artificial_table_backend_t::caching_machinery_t::caching_machinery_t(
            caching_cfeed_artificial_table_backend_t *_parent) :
        /* Set `dirtiness` to force us to load initial values. Either `dirtiness_t::all`
        or `dirtiness_t::all_stop` would work equally well here since we don't have any
        subscribers yet, but `all_stop` saves us a few CPU cycles by not diffing the old
        values against the new ones. */
        dirtiness(dirtiness_t::all_stop),
        waker(nullptr),
        parent(_parent) {
    guarantee(parent->caching_machinery == nullptr);
    parent->caching_machinery = this;
    coro_t::spawn_sometime(std::bind(&caching_machinery_t::run, this, drainer.lock()));
}

caching_cfeed_artificial_table_backend_t::caching_machinery_t::~caching_machinery_t() {
    guarantee(parent->caching_machinery == this);
    parent->caching_machinery = nullptr;
}

bool caching_cfeed_artificial_table_backend_t::caching_machinery_t::get_initial_values(
        const new_mutex_acq_t *proof,
        std::vector<ql::datum_t> *out,
        signal_t *interruptor) {
    proof->guarantee_is_holding(&mutex);

    /* This is necessary to make sure that the initial values are up-to-date. */
    if (!diff_dirty(proof, interruptor)) {
        return false;
    }

    out->clear();
    for (const auto &pair : old_values) {
        out->push_back(pair.second);
    }
    return true;
}

void caching_cfeed_artificial_table_backend_t::caching_machinery_t::run(
        auto_drainer_t::lock_t keepalive)
        THROWS_NOTHING {
    parent->set_notifications(true);
    try {
        while (true) {
            guarantee(dirtiness != dirtiness_t::none_or_some || !dirty_keys.empty(),
                "If nothing is dirty, we shouldn't have gotten here");

            bool success;
            {
                new_mutex_acq_t mutex_acq(&mutex);
                success = diff_dirty(&mutex_acq, keepalive.get_drain_signal());
            }
                
            if (success) {
                ready.pulse_if_not_already_pulsed();
            } else {
                /* Kick off any subscribers since we got an error. */
                send_all_stop();
                /* Ensure that we try again the next time around the loop. */
                dirtiness = dirtiness_t::all;
                /* Go around the loop and try again. But first wait a second so we
                don't eat too much CPU if it keeps failing. */
                nap(1000, keepalive.get_drain_signal());
                continue;
            }

            /* Wait until the next change */
            if (dirtiness == dirtiness_t::none_or_some && dirty_keys.empty()) {
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

bool caching_cfeed_artificial_table_backend_t::caching_machinery_t::diff_dirty(
        const new_mutex_acq_t *proof, signal_t *interruptor) {
    /* Copy the dirtiness flags into local variables and reset them. Resetting them now
    is important because it means that notifications that arrive while we're processing
    the current batch will be queued up instead of ignored. */
    std::set<ql::datum_t, latest_version_optional_datum_less_t> local_dirty_keys;
    dirtiness_t local_dirtiness = dirtiness_t::none_or_some;
    std::swap(dirty_keys, local_dirty_keys);
    std::swap(dirtiness, local_dirtiness);

    if (local_dirtiness != dirtiness_t::none_or_some) {
        return diff_all(local_dirtiness == dirtiness_t::all_stop, proof, interruptor);
    } else {
        for (const auto &key : local_dirty_keys) {
            if (!diff_one(key, proof, interruptor)) {
                return false;
            }
        }
        return true;
    }
}

bool caching_cfeed_artificial_table_backend_t::caching_machinery_t::diff_one(
        const ql::datum_t &key, const new_mutex_acq_t *proof, signal_t *interruptor) {
    /* Fetch new value from backend */
    ql::datum_t new_val;
    std::string error;
    if (!parent->read_row(key, interruptor, &new_val, &error)) {
        return false;
    }
    /* Fetch old value from `old_values` map */
    ql::datum_t old_val;
    auto it = old_values.find(store_key_t(key.print_primary()));
    if (it != old_values.end()) {
        old_val = it->second;
    }
    /* Update `old_values` map */
    if (!new_val.has()) {
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
    if (new_val.has() != old_val.has() || (new_val.has() && new_val != old_val)) {
        send_all_change(proof, store_key_t(key.print_primary()), old_val, new_val);
    }
    return true;
}

bool caching_cfeed_artificial_table_backend_t::caching_machinery_t::diff_all(
        bool is_break, const new_mutex_acq_t *proof, signal_t *interruptor) {
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
        auto old_it = old_values.begin();
        auto new_it = new_values.begin();
        while (old_it != old_values.end() || new_it != new_values.end()) {
            if (old_it == old_values.end() ||
                    (new_it != new_values.end() && new_it->first < old_it->first)) {
                send_all_change(proof, new_it->first, ql::datum_t(), new_it->second);
                ++new_it;
            } else if (new_it == new_values.end() ||
                    (old_it != old_values.end() && old_it->first < new_it->first)) {
                send_all_change(proof, old_it->first, old_it->second, ql::datum_t());
                ++old_it;
            } else {
                guarantee(old_it != old_values.end());
                guarantee(new_it != new_values.end());
                guarantee(old_it->first == new_it->first);
                if (old_it->second != new_it->second) {
                    send_all_change(
                        proof, old_it->first, old_it->second, new_it->second);
                }
                ++old_it;
                ++new_it;
            }
        }
    }
    old_values = std::move(new_values);
    return true;
}

bool caching_cfeed_artificial_table_backend_t::caching_machinery_t::get_values(
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
    guarantee(stream->cfeed_type() == ql::feed_type_t::not_feed);
    ql::env_t env(interruptor,
                  ql::return_empty_normal_batches_t::NO,
                  reql_version_t::LATEST);
    std::vector<ql::datum_t> datums;
    try {
        datums = stream->next_batch(&env, ql::batchspec_t::all());
    } catch (const ql::base_exc_t &) {
        return false;
    }
    guarantee(stream->is_exhausted(), "We expect ql::batchspec_t::all() to read the "
        "entire stream");
    for (const ql::datum_t &doc : datums) {
        ql::datum_t key = doc.get_field(
            datum_string_t(parent->get_primary_key_name()),
            ql::NOTHROW);
        guarantee(key.has());
        store_key_t key2(key.print_primary());
        auto pair = out->insert(std::make_pair(key2, doc));
        guarantee(pair.second);
    }
    return true;
}

scoped_ptr_t<cfeed_artificial_table_backend_t::machinery_t>
        caching_cfeed_artificial_table_backend_t::
            construct_changefeed_machinery(signal_t *interruptor) {
    scoped_ptr_t<caching_machinery_t> m(new caching_machinery_t(this));
    wait_interruptible(&m->ready, interruptor);
    return scoped_ptr_t<cfeed_artificial_table_backend_t::machinery_t>(m.release());
}

void timer_cfeed_artificial_table_backend_t::set_notifications(bool notify) {
    if (notify && !timer.has()) {
        timer.init(new repeating_timer_t(
            1000,
            [this]() { notify_all(); }
            ));
    }
    if (!notify && timer.has()) {
        timer.reset();
    }
}
