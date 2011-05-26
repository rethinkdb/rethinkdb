#ifndef __CLUSTERING_ROUTING__
#define __CLUSTERING_ROUTING__

#include "rpc/council.hpp"
#include "concurrency/rwi_lock.hpp"
#include "boost/ptr_container/ptr_map.hpp"

namespace routing {

template <class T>
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

public:
    template <class U>
    class txned_t : public U {
    public:
        txned_t(U u, access_map_t<T> *parent, T key, access_t access) 
            : U(u), txn(parent, key, access)
        { }
    private:
        txn_t txn;
    };
};

//TODO set store mailbox in the master map really needs to be a set_store_interface_mailbox_t
typedef std::map<int, set_store_mailbox_t::address_t> master_map_t;
typedef std::pair<int, set_store_mailbox_t::address_t> master_map_diff_t; 
typedef council_t<master_map_t, master_map_diff_t> master_map_council_t;

typedef std::map<int, std::pair<set_store_mailbox_t::address_t, get_store_mailbox_t::address_t> > storage_map_t;
typedef std::pair<int, std::pair<set_store_mailbox_t::address_t, get_store_mailbox_t::address_t> > storage_map_diff_t;
typedef council_t<storage_map_t, storage_map_diff_t> storage_map_council_t;

class routing_map_t {
public:
    routing_map_t () 
        : master_map_council(boost::bind(&routing_map_t::apply_master_diff, this, _1, _2), master_map_t()),
          storage_map_council(boost::bind(&routing_map_t::apply_storage_diff, this, _1, _2), storage_map_t())
    { }

    routing_map_t (master_map_council_t::address_t master_map_addr, storage_map_council_t::address_t storage_map_addr) 
        : master_map_council(boost::bind(&routing_map_t::apply_master_diff, this, _1, _2), master_map_addr),
          storage_map_council(boost::bind(&routing_map_t::apply_storage_diff, this, _1, _2), storage_map_addr)
    { }
private:
    master_map_council_t master_map_council;
    typedef access_map_t<int> master_access_map_t;
    master_access_map_t master_refcount_map;

    typedef master_access_map_t::txned_t<set_store_mailbox_t::address_t> master_map_txn_t;

public:
    master_map_txn_t get_master(int key) {
        return master_map_txn_t(master_map_council.get_value().find(key)->second, &master_refcount_map, key, rwi_read);
    }

    void add_master(int key, set_store_mailbox_t::address_t addr) {
        master_map_council.apply(master_map_diff_t(key, addr));
    }

    void apply_master_diff(const master_map_diff_t &diff, master_map_t *map) {
        master_map_txn_t write_lock(master_map_council.get_value().find(diff.first)->second, &master_refcount_map, diff.first, rwi_write);
        (*map)[diff.first] = diff.second;
    }


private:
    storage_map_council_t storage_map_council;
    typedef access_map_t<int> storage_access_map_t;
    storage_access_map_t storage_refcount_map;

    typedef storage_access_map_t::txned_t<std::pair<set_store_mailbox_t::address_t, get_store_mailbox_t::address_t> > storage_map_txn_t;

public:
    storage_map_txn_t get_storage(int key) {
        return storage_map_txn_t(storage_map_council.get_value().find(key)->second, &storage_refcount_map, key, rwi_read);
    }

    void add_storage(const storage_map_diff_t &diff) {
        storage_map_council.apply(diff);
    }

    void apply_storage_diff(const storage_map_diff_t &diff, storage_map_t *map) {
        storage_map_txn_t write_lock(storage_map_council.get_value().find(diff.first)->second, &storage_refcount_map, diff.first, rwi_write);
        (*map)[diff.first] = diff.second;
    }
};
} //namespace routing
#endif
