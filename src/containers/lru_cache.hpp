#ifndef CONTAINERS_LRU_CACHE_HPP_
#define CONTAINERS_LRU_CACHE_HPP_

#include <unordered_map>
#include <list>

#include "errors.hpp"

template <class K, class V>
class lru_cache_t {
public:
    explicit lru_cache_t(size_t max_size) : list_(), map_(), max_size_(max_size) {
        guarantee(max_size > 0);
    }

    size_t max_size() const { return max_size_; }
    size_t size() const { return map_.size(); }

    // Looks up a key, sets *out to point at its value, and marks the entry 'most
    // recently used'.  Returns false if the key is not found.
    bool lookup(const K &key, V **out) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            list_iterator list_iter = it->second;
            list_.splice(list_.end(), list_, list_iter);
            *out = &list_iter->second;
            return true;
        } else {
            *out = nullptr;
            return false;
        }
    }

    // Returns true if an insertion was performed.  (Does nothing, doesn't even update
    // LRU, if no insertion was performed.)
    bool insert(K key, V value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            return false;
        } else {
            if (map_.size() == max_size_) {
                list_iterator evictee = list_.begin();
                DEBUG_VAR size_t count = map_.erase(evictee->first);
                rassert(count == 1);
                list_.pop_front();
            }

            list_.push_back(std::make_pair(key, std::move(value)));
            list_iterator list_iter = list_.end();
            --list_iter;
            DEBUG_VAR auto res = map_.emplace(std::move(key), list_iter);
            rassert(res.second);
            return true;
        }
    }

private:
    using list_iterator = typename std::list<std::pair<K, V>>::iterator;

    // Elements are pushed onto the back and popped (when evicted) off the front.
    std::list<std::pair<K, V>> list_;
    std::unordered_map<K, list_iterator> map_;

    size_t max_size_;

    DISABLE_COPYING(lru_cache_t);
};

#endif // CONTAINERS_LRU_CACHE_HPP_

