// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_MAP_SENTRIES_HPP_
#define CONTAINERS_MAP_SENTRIES_HPP_

#include <map>
#include <set>
#include <utility>

/* `map_insertion_sentry_t` inserts a value into a map on construction, and
removes it in the destructor. */
template<class key_t, class value_t>
class map_insertion_sentry_t {
public:
    map_insertion_sentry_t() : map(nullptr) { }
    map_insertion_sentry_t(std::map<key_t, value_t> *m,
                           const key_t &key,
                           const value_t &value)
        : map(nullptr) {
        reset(m, key, value);
    }

    // For `std::vector.emplace_back` this type needs to be movable, for which we need
    // to implement our own move constructor and move assignment operator since this type
    // has a user-defined destructor. As we set `rhs.map` to `nullptr` we must also be
    // careful of self-assignment (checking `this != &rhs`).
    map_insertion_sentry_t(map_insertion_sentry_t && rhs)
        : map(rhs.map),
          it(rhs.it) {
        rhs.map = nullptr;
    }
    map_insertion_sentry_t& operator=(map_insertion_sentry_t && rhs) {
        if (this != &rhs) {
            map = rhs.map;
            it = rhs.it;

            rhs.map = nullptr;
        }
        return *this;
    }

    ~map_insertion_sentry_t() {
        reset();
    }
    map_insertion_sentry_t(const map_insertion_sentry_t &) = delete;
    map_insertion_sentry_t &operator=(const map_insertion_sentry_t &) = delete;

    void reset() {
        if (map != nullptr) {
            map->erase(it);
            map = nullptr;
        }
    }

    void reset(std::map<key_t, value_t> *m, const key_t &key, const value_t &value) {
        reset();
        map = m;
        std::pair<typename std::map<key_t, value_t>::iterator, bool> iterator_and_is_new =
            map->insert(std::make_pair(key, value));
        rassert(iterator_and_is_new.second, "value to be inserted already "
            "exists. don't do that.");
        it = iterator_and_is_new.first;
    }

private:
    std::map<key_t, value_t> *map;
    typename std::map<key_t, value_t>::iterator it;
};

/* `multimap_insertion_sentry_t` inserts a value into a multimap on
construction, and removes it in the destructor. */
template<class key_t, class value_t>
class multimap_insertion_sentry_t {
public:
    multimap_insertion_sentry_t() : map(nullptr) { }
    multimap_insertion_sentry_t(std::multimap<key_t, value_t> *m,
                                const key_t &key,
                                const value_t &value)
        : map(nullptr) {
        reset(m, key, value);
    }

    // See above.
    multimap_insertion_sentry_t(multimap_insertion_sentry_t && rhs)
        : map(rhs.map),
          it(rhs.it) {
        rhs.map = nullptr;
    }
    multimap_insertion_sentry_t& operator=(multimap_insertion_sentry_t && rhs) {
        if (this != &rhs) {
            map = rhs.map;
            it = rhs.it;

            rhs.map = nullptr;
        }
        return *this;
    }

    ~multimap_insertion_sentry_t() {
        reset();
    }
    multimap_insertion_sentry_t(const multimap_insertion_sentry_t &) = delete;
    multimap_insertion_sentry_t &operator=(const multimap_insertion_sentry_t &) = delete;

    void reset() {
        if (map != nullptr) {
            map->erase(it);
            map = nullptr;
        }
    }

    void reset(std::multimap<key_t, value_t> *m, const key_t &key, const value_t &value) {
        reset();
        map = m;
        it = map->insert(std::make_pair(key, value));
    }

private:
    std::multimap<key_t, value_t> *map;
    typename std::multimap<key_t, value_t>::iterator it;
};

/* `set_insertion_sentry_t` inserts a value into a `std::set` on construction, and
removes it in the destructor. */
template<class obj_t>
class set_insertion_sentry_t {
public:
    set_insertion_sentry_t() : set(NULL) { }
    set_insertion_sentry_t(std::set<obj_t> *s, const obj_t &obj) : set(NULL) {
        reset(s, obj);
    }
    ~set_insertion_sentry_t() {
        reset();
    }
    set_insertion_sentry_t(const set_insertion_sentry_t &) = delete;
    set_insertion_sentry_t &operator=(const set_insertion_sentry_t &) = delete;
    void reset() {
        if (set) {
            set->erase(it);
            set = NULL;
        }
    }
    void reset(std::set<obj_t> *s, const obj_t &obj) {
        reset();
        set = s;
        auto pair = set->insert(obj);
        guarantee(pair.second, "value to be inserted already exists");
        it = pair.first;
    }
private:
    std::set<obj_t> *set;
    typename std::set<obj_t>::iterator it;
};

#endif  // CONTAINERS_MAP_SENTRIES_HPP_
