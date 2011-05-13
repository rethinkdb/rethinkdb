#ifndef __CLUSTERING_ROUTING__
#define __CLUSTERING_ROUTING__

#include "rpc/council.hpp"

namespace routing {

template <class T>
class refcount_map_t :
    public home_thread_mixin_t
{
private:
    class refcount_t {
    public:
        int val;
        multi_cond_t *wait_for_0;
        refcount_t() : val(0), wait_for_0(new multi_cond_t()) {} // wait for 0 needs to be heap allocated because it can't be copied
        ~refcount_t() {
            guarantee(val == 0, "refcount_t destroyed with a refcount not equal to 0, this probably means that there's an outstanding sarc going one\n");
            wait_for_0->pulse();
            delete wait_for_0;
        }
    };
    std::map<T, refcount_t> refcounts;
public:
    void incr_refcount(T val) { 
        on_thread_t syncer(home_thread);
        //TODO make this wait if the value is being wait_for_0d
        refcounts[val].val++; 
    }
    void decr_refcount(T val) { 
        on_thread_t syncer(home_thread);
        refcounts[val].val--; 
        if (refcounts[val].val == 0) refcounts.erase(val);
    }
    void wait(T val) { 
        if (refcounts.find(val) == refcounts.end()) return;
        else refcounts[val].wait_for_0->wait();
    }

    class txn_t {
    public:
        txn_t(refcount_map_t<T> *parent, T key) 
            : parent(parent), key(key)
        { parent->incr_refcount(key); }

        /* txn_t(const txn_t &other_txn)
            : parent(other.parent)
        { parent->incr_refcount(); } */

        ~txn_t() { parent->decr_refcount(key); }
    private:
        refcount_map_t<T> *parent;
        T key;
    };

public:
    template <class U>
    class txned_t : public U {
    public:
        txned_t(U u, refcount_map_t<T> *parent, T key) 
            : U(u), txn(parent, key)
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
    typedef refcount_map_t<int> master_refcount_map_t;
    master_refcount_map_t master_refcount_map;

    typedef master_refcount_map_t::txned_t<set_store_mailbox_t::address_t> master_map_txn_t;

public:
    master_map_txn_t get_master(int key) {
        return master_map_txn_t(master_map_council.get_value().find(key)->second, &master_refcount_map, key);
    }

    void add_master(int key, set_store_mailbox_t::address_t addr) {
        master_map_council.apply(master_map_diff_t(key, addr));
    }

    void apply_master_diff(const master_map_diff_t &diff, master_map_t *map) {
        master_refcount_map.wait(diff.first);
        (*map)[diff.first] = diff.second;
    }


private:
    storage_map_council_t storage_map_council;
    typedef refcount_map_t<int> storage_refcount_map_t;
    storage_refcount_map_t storage_refcount_map;

    typedef storage_refcount_map_t::txned_t<std::pair<set_store_mailbox_t::address_t, get_store_mailbox_t::address_t> > storage_map_txn_t;

public:
    storage_map_txn_t get_storage(int key) {
        return storage_map_txn_t(storage_map_council.get_value().find(key)->second, &storage_refcount_map, key);
    }

    void add_storage(const storage_map_diff_t &diff) {
        storage_map_council.apply(diff);
    }

    void apply_storage_diff(const storage_map_diff_t &diff, storage_map_t *map) {
        storage_refcount_map.wait(diff.first);
        (*map)[diff.first] = diff.second;
    }
};
} //namespace routing
#endif
