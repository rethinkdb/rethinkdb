#ifndef __CLUSTERING_ROUTING__
#define __CLUSTERING_ROUTING__

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "rpc/council.hpp"
#include "concurrency/rwi_lock.hpp"
#include "clustering/map_council.hpp"

namespace routing {

// TODO: What is this code and why is it commented out?  I will delete
// this if you don't tell me.

//
//template <class T>
//class access_map_t :
//    public home_thread_mixin_t
//{
//private:
//    boost::ptr_map<T, rwi_lock_t> lock_map;
//public:
//    void lock(T key, access_t access) {
//        lock_map[key].co_lock(access);
//    }
//    void unlock(T key) {
//        lock_map[key].unlock();
//    }
//
//    class txn_t {
//    public:
//        txn_t(access_map_t<T> *parent, T key, access_t access) 
//            : parent(parent), key(key)
//        { 
//            parent->lock(key, access); 
//        }
//
//        ~txn_t() { 
//            parent->unlock(key); 
//        }
//    private:
//        access_map_t<T> *parent;
//        T key;
//    };
//
//public:
//    template <class U>
//    class txned_t : public U {
//    public:
//        txned_t(U u, access_map_t<T> *parent, T key, access_t access) 
//            : U(u), txn(parent, key, access)
//        { }
//    private:
//        txn_t txn;
//    };
//};
//
///* Master map definition */
////TODO set store mailbox in the master map really needs to be a set_store_interface_mailbox_t
//typedef std::map<int, set_store_mailbox_t::address_t> master_map_t;
//typedef std::pair<int, set_store_mailbox_t::address_t> master_map_diff_t; 
//
//class master_map_council_t : public council_t<master_map_t, master_map_diff_t>
//{
//    typedef access_map_t<int> master_access_map_t;
//    master_access_map_t master_refcount_map;
//public:
//public:
//    typedef master_access_map_t::txned_t<set_store_mailbox_t::address_t> master_map_txn_t;
//
//    void update(const master_map_diff_t& diff, master_map_t *map) {
//        master_access_map_t::txn_t txn(&master_refcount_map, diff.first, rwi_write);
//        (*map)[diff.first] = diff.second;
//    }
//
//    master_map_txn_t get_master(int key) {
//        return master_map_txn_t(get_value().find(key)->second, &master_refcount_map, key, rwi_read);
//    }
//};
////typedef council_t<master_map_t, master_map_diff_t> master_map_council_t;
//
//typedef std::map<int, std::pair<set_store_mailbox_t::address_t, get_store_mailbox_t::address_t> > storage_map_t;
//typedef std::pair<int, std::pair<set_store_mailbox_t::address_t, get_store_mailbox_t::address_t> > storage_map_diff_t;
//
///* Storage map definition */
//class storage_map_council_t : public council_t<storage_map_t, storage_map_diff_t>
//{
//    typedef access_map_t<int> storage_access_map_t;
//    storage_access_map_t storage_refcount_map;
//public:
//    typedef storage_access_map_t::txned_t<std::pair<set_store_mailbox_t::address_t, get_store_mailbox_t::address_t> > storage_map_txn_t;
//
//    void update(const storage_map_diff_t& diff, storage_map_t *map) {
//        storage_access_map_t::txn_t txn(&storage_refcount_map, diff.first, rwi_write);
//        (*map)[diff.first] = diff.second;
//    }
//
//    storage_map_txn_t get_storage(int key) {
//        return storage_map_txn_t(get_value().find(key)->second, &storage_refcount_map, key, rwi_read);
//    }
//};
////typedef council_t<storage_map_t, storage_map_diff_t> storage_map_council_t;
//
//class routing_map_t {
//public:
//    routing_map_t () 
//        : master_map_council(master_map_t()),
//          storage_map_council(storage_map_t())
//    { }
//
//    routing_map_t (master_map_council_t::address_t master_map_addr, storage_map_council_t::address_t storage_map_addr) 
//        : master_map_council(master_map_addr),
//          storage_map_council(storage_map_addr)
//    { }
//private:
//    master_map_council_t master_map_council;
//
//public:
//    master_map_council_t::master_map_txn_t get_master(int key) {
//        return master_map_txn_t(master_map_council.get_value().find(key)->second, &master_refcount_map, key, rwi_read);
//    }
//
//    void add_master(int key, set_store_mailbox_t::address_t addr) {
//        master_map_council.apply(master_map_diff_t(key, addr));
//    }
//
//    /* void apply_master_diff(const master_map_diff_t &diff, master_map_t *map) {
//        master_map_txn_t write_lock(master_map_council.get_value().find(diff.first)->second, &master_refcount_map, diff.first, rwi_write);
//        (*map)[diff.first] = diff.second;
//    } */
//
//
//private:
//    storage_map_council_t storage_map_council;
//    typedef access_map_t<int> storage_access_map_t;
//    storage_access_map_t storage_refcount_map;
//
//    typedef storage_access_map_t::txned_t<std::pair<set_store_mailbox_t::address_t, get_store_mailbox_t::address_t> > storage_map_txn_t;
//
//public:
//    storage_map_txn_t get_storage(int key) {
//        return storage_map_txn_t(storage_map_council.get_value().find(key)->second, &storage_refcount_map, key, rwi_read);
//    }
//
//    void add_storage(const storage_map_diff_t &diff) {
//        storage_map_council.apply(diff);
//    }
//
//    void apply_storage_diff(const storage_map_diff_t &diff, storage_map_t *map) {
//        storage_map_txn_t write_lock(storage_map_council.get_value().find(diff.first)->second, &storage_refcount_map, diff.first, rwi_write);
//        (*map)[diff.first] = diff.second;
//    }
//};

class routing_map_t {
public:
    typedef map_council_t<int, set_store_mailbox_t::address_t> master_map_t;
    typedef map_council_t<int, std::pair<set_store_mailbox_t::address_t, get_store_mailbox_t::address_t> > storage_map_t;

    master_map_t master_map;
    storage_map_t storage_map;

public:
    routing_map_t() { }


    typedef std::pair<master_map_t::address_t, storage_map_t::address_t> address_t;

    routing_map_t(address_t addr)
        : master_map(addr.first), storage_map(addr.second)
    { }


    address_t get_address() {
        return make_pair(master_map_t::address_t(&master_map), storage_map_t::address_t(&storage_map));
    }
};
} //namespace routing

#endif
