#ifndef __CLUSTERING_MAP_COUNCIL_HPP__
#define __CLUSTERING_MAP_COUNCIL_HPP__

#include "rpc/council.hpp"

/* template <class T>
class access_map_t :
    public home_thread_mixin_t
{
private:
    boost::ptr_map<T, rwi_lock_t> lock_map;
public:
    void lock(T key, access_t access) {
        lock_map[key].co_lock(access);
    }
    void unlock(T key) {
        lock_map[key].unlock();
    }

    class txn_t {
    public:
        txn_t(access_map_t<T> *parent, T key, access_t access) 
            : parent(parent), key(key)
        { 
            parent->lock(key, access); 
        }

        ~txn_t() { 
            parent->unlock(key); 
        }
    private:
        access_map_t<T> *parent;
        T key;
    };
}; */

template<class K, class V>
class map_council_t : public council_t<std::map<K,V>, std::pair<K,V> >
{
    typedef std::map<K,V> value_t;
    typedef std::pair<K,V> diff_t;

public:
    typedef typename std::map<K, V>::iterator iterator;
    typedef typename std::map<K, V>::const_iterator const_iterator;

    map_council_t() {}
    map_council_t(const typename council_t<value_t, diff_t>::address_t &addr)
        : council_t<value_t, diff_t>(addr)
    { }
    void update(const diff_t& diff, value_t *value) {
        // for now we don't support changing values it will require attaching rwi_locks to everything
        rassert(value->find(diff.first) == value->end());
        (*value)[diff.first] = diff.second;
    }
    void insert(K k, V v) {
        apply(std::make_pair(k,v));
    }
    const_iterator find(K &k) const {
        return council_t<std::map<K,V>, std::pair<K,V> >::get_value().find(k);
    }

    /* iterator begin() {
        return council_t<std::map<K,V>, std::pair<K,V> >::get_value().begin();
    } */

    const_iterator begin() {
        return council_t<std::map<K,V>, std::pair<K,V> >::get_value().begin();
    }

    /* iterator end() {
        return council_t<std::map<K,V>, std::pair<K,V> >::get_value().end();
    } */

    const_iterator end() {
        return council_t<std::map<K,V>, std::pair<K,V> >::get_value().end();
    }

    /* typename value_t::const_iterator end() const {
        return council_t<std::map<K,V>, std::pair<K,V> >::get_value().end();
    } */
};

#endif
