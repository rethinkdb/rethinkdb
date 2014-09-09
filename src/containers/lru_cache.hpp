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
    typedef V& reference;
    typedef const V& const_reference;
    
    typedef std::pair<K, V> entry_pair_t;
    typedef std::list<entry_pair_t> cache_list_t;
    typedef std::map<K, typename cache_list_t::iterator> cache_map_t;
    
    typedef typename cache_list_t::iterator iterator;
    typedef typename cache_list_t::const_iterator const_iterator;
    typedef typename cache_list_t::reverse_iterator reverse_iterator;
    typedef typename cache_list_t::const_reverse_iterator const_reverse_iterator;
    
private:
    cache_list_t _cache_list;
    cache_map_t _cache_map;
    size_t _max;
public:
    lru_cache_t(size_t max) : _cache_list(), _cache_map(), _max(max) {}
    
    iterator begin() { return _cache_list.begin(); }
    const_iterator begin() const { return _cache_list.cbegin(); }
    const_iterator cbegin() const { return _cache_list.cbegin(); }
    iterator end() { return _cache_list.end(); }
    const_iterator end() const { return _cache_list.cend(); }
    const_iterator cend() const { return _cache_list.cend(); }

    reverse_iterator rbegin() { return _cache_list.rbegin(); }
    const_reverse_iterator rbegin() const { return _cache_list.crbegin(); }
    const_reverse_iterator crbegin() const { return _cache_list.crbegin(); }
    reverse_iterator rend() { return _cache_list.rend(); }
    const_reverse_iterator rend() const { return _cache_list.crend(); }
    const_reverse_iterator crend() const { return _cache_list.crend(); }
    
    size_t size() const { return _cache_list.size(); }
    size_t max_size() const { return _max; }
    bool empty() const { return _cache_list.empty(); }
    
    V& operator[](const K& key) {
        auto search = _cache_map.find(key);
        if (search != _cache_map.end()) {
            bump(search);
            return search->second->second;
        } else {
            return insert(std::move(K(key)));
        }
    }
    V& operator[](K&& key) {
        auto search = _cache_map.find(key);
        if (search != _cache_map.end()) {
            bump(search);
            return search->second->second;
        } else {
            return insert(key);
        }
    }
    iterator find(const K& key) {
        auto search = _cache_map.find(key);
        if (search != _cache_map.end()) {
            bump(search);
            return search->second;
        } else {
            return _cache_list.end();
        }
    }
private:
    V& insert(const K & key) {
        _cache_list.push_front(std::make_pair(key, V()));
        _cache_map[key] = _cache_list.begin();
        if (_cache_list.size() > _max) {
            _cache_map.erase(_cache_list.back().first);
            _cache_list.pop_back();
        }
        return _cache_list.begin()->second;
    }
    V& insert(K&& key) {
        _cache_list.push_front(std::make_pair(std::move(key), V()));
        _cache_map[key] = _cache_list.begin();
        if (_cache_list.size() > _max) {
            _cache_map.erase(_cache_list.back().first);
            _cache_list.pop_back();
        }
        return _cache_list.begin()->second;
    }
    void bump(typename cache_map_t::iterator &it) {
        entry_pair_t pair = *(it->second);
        _cache_list.erase(it->second);
        _cache_list.push_front(std::move(pair));
        it->second = _cache_list.begin();
    }
};

#endif // CONTAINERS_LRU_CACHE_HPP_

