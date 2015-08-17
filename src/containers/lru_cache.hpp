#ifndef CONTAINERS_LRU_CACHE_HPP_
#define CONTAINERS_LRU_CACHE_HPP_

#include <map>
#include <list>

#include "errors.hpp"

template <typename K, typename V>
class lru_cache_t {
public:
    typedef K key_type;
    typedef V value_type;
    typedef V &reference;
    typedef const V &const_reference;
    
    typedef std::pair<K, V> entry_pair_t;
    typedef std::list<entry_pair_t> cache_list_t;
    typedef std::map<K, typename cache_list_t::iterator> cache_map_t;
    
    typedef typename cache_list_t::iterator iterator;
    typedef typename cache_list_t::const_iterator const_iterator;
    typedef typename cache_list_t::reverse_iterator reverse_iterator;
    typedef typename cache_list_t::const_reverse_iterator const_reverse_iterator;
    
private:
    // Cache entries, ordered by access time (so cache_list_.begin() points to the
    // most recently accessed entry).
    cache_list_t cache_list_;
    // Map from key values to position in cache list.
    cache_map_t cache_map_;
    size_t _max;
public:
    explicit lru_cache_t(size_t max) : cache_list_(), cache_map_(), _max(max) {}
    
    // These iterators can be invalidated if someone uses `[]` or `find`; specifically
    // it can be the case (particularly with the end positions) that the object you're
    // looking at can be recycled by the LRU cache as you insert new entries.
    iterator begin() { return cache_list_.begin(); }
    const_iterator begin() const { return cache_list_.cbegin(); }
    const_iterator cbegin() const { return cache_list_.cbegin(); }
    iterator end() { return cache_list_.end(); }
    const_iterator end() const { return cache_list_.cend(); }
    const_iterator cend() const { return cache_list_.cend(); }

    reverse_iterator rbegin() { return cache_list_.rbegin(); }
    const_reverse_iterator rbegin() const { return cache_list_.crbegin(); }
    const_reverse_iterator crbegin() const { return cache_list_.crbegin(); }
    reverse_iterator rend() { return cache_list_.rend(); }
    const_reverse_iterator rend() const { return cache_list_.crend(); }
    const_reverse_iterator crend() const { return cache_list_.crend(); }
    
    size_t size() const { return cache_list_.size(); }
    size_t max_size() const { return _max; }
    bool empty() const { return cache_list_.empty(); }
    
    V &operator[](const K &key) {
        auto search = cache_map_.find(key);
        if (search != cache_map_.end()) {
            bump(search);
            return search->second->second;
        } else {
            return insert(K(key));
        }
    }
    V &operator[](K &&key) {
        auto search = cache_map_.find(key);
        if (search != cache_map_.end()) {
            bump(search);
            return search->second->second;
        } else {
            return insert(std::move(key));
        }
    }
    iterator find(const K &key) {
        auto search = cache_map_.find(key);
        if (search != cache_map_.end()) {
            bump(search);
            return search->second;
        } else {
            return cache_list_.end();
        }
    }
private:
    V &insert(const K &key) {
        cache_list_.push_front(std::make_pair(key, V()));
        cache_map_[key] = cache_list_.begin();
        if (cache_list_.size() > _max) {
            cache_map_.erase(cache_list_.back().first);
            cache_list_.pop_back();
        }
        return cache_list_.begin()->second;
    }
    V &insert(K &&key) {
        cache_list_.push_front(std::make_pair(std::move(key), V()));
        cache_map_[key] = cache_list_.begin();
        if (cache_list_.size() > _max) {
            cache_map_.erase(cache_list_.back().first);
            cache_list_.pop_back();
        }
        return cache_list_.begin()->second;
    }
    void bump(const typename cache_map_t::iterator &it) {
        entry_pair_t pair = *(it->second);
        cache_list_.erase(it->second);
        cache_list_.push_front(std::move(pair));
        it->second = cache_list_.begin();
    }
};

#endif // CONTAINERS_LRU_CACHE_HPP_

