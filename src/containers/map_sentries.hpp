#ifndef CONTAINERS_MAP_SENTRIES_HPP_
#define CONTAINERS_MAP_SENTRIES_HPP_

#include <map>
#include <utility>

/* `map_insertion_sentry_t` inserts a value into a map on construction, and
removes it in the destructor. */
template<class key_t, class value_t>
class map_insertion_sentry_t {
public:
    map_insertion_sentry_t() : map(NULL) { }
    map_insertion_sentry_t(std::map<key_t, value_t> *m, const key_t &key, const value_t &value) : map(NULL) {
        reset(m, key, value);
    }
    ~map_insertion_sentry_t() {
        reset();
    }
    void reset() {
        if (map) {
            map->erase(it);
            map = NULL;
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
    multimap_insertion_sentry_t() : map(NULL) { }
    multimap_insertion_sentry_t(std::multimap<key_t, value_t> *m, const key_t &key, const value_t &value) : map(NULL) {
        reset(m, key, value);
    }
    ~multimap_insertion_sentry_t() {
        reset();
    }
    void reset() {
        if (map) {
            map->erase(it);
            map = NULL;
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




#endif  // CONTAINERS_MAP_SENTRIES_HPP_
