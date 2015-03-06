// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "concurrency/watchable_transform.hpp"

#include "arch/runtime/coroutines.hpp"
#include "stl_utils.hpp"

template<class key1_t, class value1_t, class key2_t, class value2_t>
watchable_map_transform_t<key1_t, value1_t, key2_t, value2_t>::watchable_map_transform_t(
        watchable_map_t<key1_t, value1_t> *_inner) :
    inner(_inner),
    all_subs(inner,
        [this](const key1_t &key1, const value1_t *value1) {
            rwi_lock_assertion_t::write_acq_t write_acq(&(this->rwi_lock));
            key2_t key2;
            if (this->key_1_to_2(key1, &key2)) {
                if (value1 != nullptr) {
                    const value2_t *value2;
                    this->value_1_to_2(value1, &value2);
                    this->do_notify_change(key2, value2, &write_acq);
                } else {
                    this->do_notify_change(key2, nullptr, &write_acq);
                }
            }
        }, false)
    { }

template<class key1_t, class value1_t, class key2_t, class value2_t>
std::map<key2_t, value2_t>
watchable_map_transform_t<key1_t, value1_t, key2_t, value2_t>::get_all() {
    std::map<key2_t, value2_t> map2;
    read_all([&](const key2_t &key2, const value2_t *value2) {
        auto pair = map2.insert(std::make_pair(key2, *value2));
        guarantee(pair.second, "key_1_to_2 created collision");
    });
    return map2;
}

template<class key1_t, class value1_t, class key2_t, class value2_t>
boost::optional<value2_t>
watchable_map_transform_t<key1_t, value1_t, key2_t, value2_t>::get_key(
        const key2_t &key2) {
    key1_t key1;
    if (!key_2_to_1(key2, &key1)) {
        return boost::optional<value2_t>();
    }
    boost::optional<value2_t> res;
    inner->read_key(key1, [&](const value1_t *value1) {
        if (value1 != nullptr) {
            const value2_t *value2;
            this->value_1_to_2(value1, &value2);
            if (value2 != nullptr) {
                res = boost::optional<value2_t>(*value2);
            }
        }
    });
    return res;
}

template<class key1_t, class value1_t, class key2_t, class value2_t>
void watchable_map_transform_t<key1_t, value1_t, key2_t, value2_t>::read_all(
        const std::function<void(const key2_t &, const value2_t *)> &cb) {
    inner->read_all([&](const key1_t &key1, const value1_t *value1) {
        key2_t key2;
        if (this->key_1_to_2(key1, &key2)) {
            const value2_t *value2;
            this->value_1_to_2(value1, &value2);
            if (value2 != nullptr) {
                cb(key2, value2);
            }
        }
    });
}

template<class key1_t, class value1_t, class key2_t, class value2_t>
void watchable_map_transform_t<key1_t, value1_t, key2_t, value2_t>::read_key(
        const key2_t &key2, const std::function<void(const value2_t *)> &cb) {
    key1_t key1;
    if (!key_2_to_1(key2, &key1)) {
        cb(nullptr);
        return;
    }
    inner->read_key(key1, [&](const value1_t *value1) {
        if (value1 != nullptr) {
            const value2_t *value2;
            this->value_1_to_2(value1, &value2);
            cb(value2);
        } else {
            cb(nullptr);
        }
    });
}

template<class key1_t, class value1_t, class key2_t, class value2_t>
rwi_lock_assertion_t *
watchable_map_transform_t<key1_t, value1_t, key2_t, value2_t>::get_rwi_lock() {
    return &rwi_lock;
}

template<class key_t, class value_t>
clone_ptr_t<watchable_t<boost::optional<value_t> > > get_watchable_for_key(
        watchable_map_t<key_t, value_t> *map,
        const key_t &key) {
    class w_t : public watchable_t<boost::optional<value_t> > {
    public:
        w_t(watchable_map_t<key_t, value_t> *_map, const key_t &_key) :
            map(_map), key(_key),
            subs(map, key,
                [this](const value_t *) {
                     rwi_lock_assertion_t::write_acq_t acq(&(this->rwi_lock));
                    publisher.publish([](const std::function<void()> &f) { f(); });
                },
                false)
            { }
        w_t *clone() const {
            return new w_t(map, key);
        }
        boost::optional<value_t> get() {
            return map->get_key(key);
        }
        void apply_read(
                const std::function<void(const boost::optional<value_t> *)> &read) {
            boost::optional<value_t> value = get();
            read(&value);
        }
        publisher_t<std::function<void()> > *get_publisher() {
            return publisher.get_publisher();
        }
        rwi_lock_assertion_t *get_rwi_lock_assertion() {
            return &rwi_lock;
        }
    private:
        publisher_controller_t<std::function<void()> > publisher;
        rwi_lock_assertion_t rwi_lock;
        watchable_map_t<key_t, value_t> *map;
        key_t key;
        typename watchable_map_t<key_t, value_t>::key_subs_t subs;
    };
    return clone_ptr_t<watchable_t<boost::optional<value_t> > >(new w_t(map, key));
}

template<class key_t, class value_t>
watchable_map_entry_copier_t<key_t, value_t>::watchable_map_entry_copier_t(
        watchable_map_var_t<key_t, value_t> *_map,
        const key_t &_key,
        clone_ptr_t<watchable_t<value_t> > _value,
        bool _remove_when_done) :
    map(_map), key(_key), value(_value), remove_when_done(_remove_when_done),
    subs([this]() {
        map->set_key_no_equals(key, value->get());
    })
{
    typename watchable_t<value_t>::freeze_t freeze(value);
    map->set_key_no_equals(key, value->get());
    subs.reset(value, &freeze);
}

template<class key_t, class value_t>
watchable_map_entry_copier_t<key_t, value_t>::~watchable_map_entry_copier_t() {
    if (remove_when_done) {
        map->delete_key(key);
    }
}

template<class tag_t, class key_t, class value_t>
watchable_map_combiner_t<tag_t, key_t, value_t>::source_t::source_t(
        watchable_map_combiner_t *_parent,
        const tag_t &tag,
        watchable_map_t<key_t, value_t> *inner) :
    parent(_parent),
    sentry(&parent->map, tag, inner),
    subs(inner, std::bind(&source_t::on_change, this, ph::_1, ph::_2), true)
    { }

template<class tag_t, class key_t, class value_t>
void watchable_map_combiner_t<tag_t, key_t, value_t>::source_t::on_change(
        const key_t &key, const value_t *value) {
    rwi_lock_assertion_t::write_acq_t write_acq(&parent->rwi_lock);
    parent->notify_change(std::make_pair(sentry.get_key(), key), value, &write_acq);
}

template<class tag_t, class key_t, class value_t>
std::map<std::pair<tag_t, key_t>, value_t>
watchable_map_combiner_t<tag_t, key_t, value_t>::get_all() {
    std::map<std::pair<tag_t, key_t>, value_t> res;
    read_all([&](const std::pair<tag_t, key_t> &key, const value_t *value) {
        res.insert(std::make_pair(key, *value));
    });
    return res;
}

template<class tag_t, class key_t, class value_t>
boost::optional<value_t> watchable_map_combiner_t<tag_t, key_t, value_t>::get_key(
        const std::pair<tag_t, key_t> &key) {
    auto it = map.find(key.first);
    if (it == map.end()) {
        return boost::none;
    }
    return it->second->get_key(key.second);
}

template<class tag_t, class key_t, class value_t>
void watchable_map_combiner_t<tag_t, key_t, value_t>::read_all(
        const std::function<void(
            const std::pair<tag_t, key_t> &, const value_t *)> &callback) {
    for (const auto &pair : map) {
        pair.second->read_all([&](const key_t &key, const value_t *value) {
            callback(std::make_pair(pair.first, key), value);
        });
    }
}

template<class tag_t, class key_t, class value_t>
void watchable_map_combiner_t<tag_t, key_t, value_t>::read_key(
        const std::pair<tag_t, key_t> &key,
        const std::function<void(const value_t *)> &callback) {
    auto it = map.find(key.first);
    if (it == map.end()) {
        callback(nullptr);
    }
    it->second->read_key(key.second, callback);
}

