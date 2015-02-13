// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "concurrency/watchable_map.hpp"

#include "concurrency/cond_var.hpp"
#include "concurrency/interruptor.hpp"

template<class key_t, class value_t>
watchable_map_t<key_t, value_t>::all_subs_t::all_subs_t(
        watchable_map_t<key_t, value_t> *map,
        const std::function<void(const key_t &key, const value_t *maybe_value)> &cb,
        bool initial_call) :
        subscription(cb) {
    rwi_lock_assertion_t::read_acq_t acq(map->get_rwi_lock());
    subscription.reset(map->all_subs_publisher.get_publisher());
    if (initial_call) {
        map->read_all(cb);
    }
}

template<class key_t, class value_t>
watchable_map_t<key_t, value_t>::key_subs_t::key_subs_t(
        watchable_map_t<key_t, value_t> *map,
        const key_t &key,
        const std::function<void(const value_t *maybe_value)> &cb,
        bool initial_call) :
        sentry(&map->key_subs_map, key, cb) {
    if (initial_call) {
        map->read_key(key, cb);
    }
}

template<class key_t, class value_t>
template<class callable_t>
void watchable_map_t<key_t, value_t>::run_key_until_satisfied(
        const key_t &key,
        const callable_t &fun,
        signal_t *interruptor) {
    assert_thread();
    cond_t ok;
    key_subs_t subs(this, key,
        [&](const value_t *new_value) {
            if (fun(new_value)) {
                ok.pulse();
            }
        }, true);
    wait_interruptible(&ok, interruptor);
}

template<class key_t, class value_t>
template<class callable_t>
void watchable_map_t<key_t, value_t>::run_all_until_satisfied(
        const callable_t &fun,
        signal_t *interruptor) {
    assert_thread();
    cond_t *notify = nullptr;
    all_subs_t subs(this,
        [&notify](const key_t &, const value_t *) {
            if (notify != nullptr) {
                notify->pulse_if_not_already_pulsed();
            }
        },
        false);
    while (true) {
        cond_t cond;
        assignment_sentry_t<cond_t *> sentry(&notify, &cond);
        if (fun(this)) {
            return;
        }
        wait_interruptible(&cond, interruptor);
    }
}

template<class key_t, class value_t>
void watchable_map_t<key_t, value_t>::notify_change(
        const key_t &key,
        const value_t *new_value,
        rwi_lock_assertion_t::write_acq_t *acq) {
    ASSERT_FINITE_CORO_WAITING;
    acq->assert_is_holding(get_rwi_lock());
    all_subs_publisher.publish(
        [&](const std::function<void(const key_t &, const value_t *)> &callback) {
            callback(key, new_value);
        });
    for (auto it = key_subs_map.lower_bound(key);
            it != key_subs_map.upper_bound(key);
            ++it) {
        it->second(new_value);
    }
}

template<class key_t, class value_t>
watchable_map_var_t<key_t, value_t>::entry_t::entry_t(
        watchable_map_var_t *p, const key_t &key, const value_t &value) :
    parent(p)
{
    rwi_lock_assertion_t::write_acq_t write_acq(&parent->rwi_lock);
    auto pair = parent->map.insert(std::make_pair(key, value));
    guarantee(pair.second, "key for entry_t already exists");
    iterator = pair.first;
    parent->notify_change(iterator->first, &iterator->second, &write_acq);
}

template<class key_t, class value_t>
watchable_map_var_t<key_t, value_t>::entry_t::entry_t(entry_t &&other) :
    parent(other.parent), iterator(other.iterator)
{
    other.parent = nullptr;
}

template<class key_t, class value_t>
watchable_map_var_t<key_t, value_t>::entry_t::~entry_t() {
    if (parent != nullptr) {
        *this = entry_t();
    }
}

template<class key_t, class value_t>
typename watchable_map_var_t<key_t, value_t>::entry_t &
watchable_map_var_t<key_t, value_t>::entry_t::operator=(entry_t &&other) {
    if (this != &other) {
        if (parent != nullptr) {
            rwi_lock_assertion_t::write_acq_t write_acq(&parent->rwi_lock);
            key_t key = iterator->first;
            parent->map.erase(iterator);
            parent->notify_change(key, nullptr, &write_acq);
        }
        parent = other.parent;
        iterator = other.iterator;
        other.parent = nullptr;
    }
    return *this;
}

template <class key_t, class value_t>
watchable_map_var_t<key_t, value_t>::watchable_map_var_t(
        std::map<key_t, value_t> &&source) :
    map(std::move(source)) { }

template<class key_t, class value_t>
std::map<key_t, value_t> watchable_map_var_t<key_t, value_t>::get_all() {
    return map;
}

template<class key_t, class value_t>
boost::optional<value_t> watchable_map_var_t<key_t, value_t>::get_key(const key_t &key) {
    auto it = map.find(key);
    if (it == map.end()) {
        return boost::optional<value_t>();
    } else {
        return boost::optional<value_t>(it->second);
    }
}

template<class key_t, class value_t>
void watchable_map_var_t<key_t, value_t>::read_all(
        const std::function<void(const key_t &, const value_t *)> &fun) {
    ASSERT_FINITE_CORO_WAITING;
    for (const auto &pair : map) {
        fun(pair.first, &pair.second);
    }
}

template<class key_t, class value_t>
void watchable_map_var_t<key_t, value_t>::read_key(
        const key_t &key,
        const std::function<void(const value_t *)> &fun) {
    ASSERT_FINITE_CORO_WAITING;
    auto it = map.find(key);
    if (it == map.end()) {
        fun(nullptr);
    } else {
        fun(&it->second);
    }
}

template<class key_t, class value_t>
void watchable_map_var_t<key_t, value_t>::set_all(
        const std::map<key_t, value_t> &new_value) {
    for (auto it = map.begin(); it != map.end();) {
        auto jt = it;
        ++it;
        if (new_value.count(jt->first) == 0) {
            delete_key(jt->first);
        }
    }
    for (auto it = new_value.begin(); it != new_value.end(); ++it) {
        set_key_no_equals(it->first, it->second);
    }
}

template<class key_t, class value_t>
void watchable_map_var_t<key_t, value_t>::set_key(
        const key_t &key, const value_t &new_value) {
    rwi_lock_assertion_t::write_acq_t write_acq(&rwi_lock);
    auto it = map.find(key);
    if (it == map.end()) {
        map.insert(std::make_pair(key, new_value));
        watchable_map_t<key_t, value_t>::notify_change(key, &new_value, &write_acq);
    } else {
        if (it->second == new_value) {
            return;
        }
        it->second = new_value;
        watchable_map_t<key_t, value_t>::notify_change(key, &new_value, &write_acq);
    }
}

template<class key_t, class value_t>
void watchable_map_var_t<key_t, value_t>::set_key_no_equals(
        const key_t &key, const value_t &new_value) {
    rwi_lock_assertion_t::write_acq_t write_acq(&rwi_lock);
    map[key] = new_value;
    watchable_map_t<key_t, value_t>::notify_change(key, &new_value, &write_acq);
}

template<class key_t, class value_t>
void watchable_map_var_t<key_t, value_t>::delete_key(const key_t &key) {
    rwi_lock_assertion_t::write_acq_t write_acq(&rwi_lock);
    map.erase(key);
    watchable_map_t<key_t, value_t>::notify_change(key, nullptr, &write_acq);
}

template<class key_t, class value_t>
void watchable_map_var_t<key_t, value_t>::change_key(
        const key_t &key,
        const std::function<bool(bool *exists, value_t *value)> &callback) {
    rwi_lock_assertion_t::write_acq_t write_acq(&rwi_lock);
    auto it = map.find(key);
    if (it != map.end()) {
        bool exists = true;
        if (!callback(&exists, &it->second)) {
            guarantee(exists);
            return;
        }
        if (exists) {
            watchable_map_t<key_t, value_t>::notify_change(
                key, &it->second, &write_acq);
        } else {
            map.erase(it);
            watchable_map_t<key_t, value_t>::notify_change(key, nullptr, &write_acq);
        }
    } else {
        bool exists = false;
        value_t value_buffer;
        if (!callback(&exists, &value_buffer)) {
            guarantee(!exists);
            return;
        }
        if (exists) {
            auto pair = map.insert(std::make_pair(key, std::move(value_buffer)));
            watchable_map_t<key_t, value_t>::notify_change(
                key, &pair.first->second, &write_acq);
        }
    }
}

