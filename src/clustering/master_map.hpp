#ifndef _CLUSTERING_MASTER_MAP_HPP_
#define _CLUSTERING_MASTER_MAP_HPP_
#include <map>
#include "clustering/cluster.hpp"
#include "clustering/cluster_store.hpp"
#include "clustering/rpc.hpp"
#include "concurrency/cond_var.hpp"
#include "clustering/cluster_store.hpp"
#include <boost/bind.hpp>

typedef unsigned int hash_t;

//typedef set_store_interface_mailbox_t::address_t store_mailbox_t;
//typedef store_mailbox_t::address_t set_store_t;
//
//typedef sync_mailbox_t<void(hash_t, set_store_t)> set_master_mailbox_t;
//typedef sync_mailbox_t<void(hash_t, set_store_t)> take_master_mailbox_t;
//
//typedef sync_mailbox_t<void(hash_t, set_store_t)> add_storage_mailbox_t;
//typedef sync_mailbox_t<void(hash_t, set_store_t)> remove_storage_mailbox_t;
//
//typedef sync_mailbox_t<void()> move_choke_mailbox_t;
//
//typedef sync_mailbox_t<void()> downgrade_redundancy_mailbox_t;

class hasher_t {
public:
    hasher_t() : 
        choke(1)
        //move_choke_mailbox(boost::bind(&hasher_t::move_choke, this))
    {}
    hash_t hash(store_key_t k) { 
        unsigned int res = 0;
        for (int i = 0; i < k.size; i++) {
            res ^= res << 18;
            res += res >> 7;
            res += k.contents[i];
        }
        return res;
    }
private:
    int choke;
    //move_choke_mailbox_t move_choke_mailbox;
    void move_choke() { choke++; }
};

/* the master_map keeps track of who is the master for a given key */
/* class master_map_t {
private:
    std::map<hash_t, set_store_t> inner_map; */

/* The master map is responsible for managing and communicating with the other peers about responsibility changes */
//private:
    //set_master_mailbox_t set_master_mailbox;
    /* void set(hash_t hash, set_store_t address) {
        inner_map[hash] = address;
    }

public:
    set_store_t get(hash_t hash) {
        guarantee(inner_map.find(hash) != inner_map.end(), "Trying to get nonexistant master");
        return inner_map[hash];
    }

public:
    master_map_t()
        //: set_master_mailbox(boost::bind(&master_map_t::set, this, _1, _2))
    {}
}; */

/* the storage_map keeps track of who is storing certain keys */
class set_store_t;
class storage_map_t {
private:
    std::map<int, set_store_t*> inner_map;

private:
    class redundant_hasher_t {
    public:
        int log_buckets;
        int log_redundancy;
        hasher_t hasher;
    public:
        class iterator {
        public:
            int val;
            int modulos;
            int log_buckets;
            bool end;
        private:
            friend class redundant_hasher_t;
            iterator(int val, int modulos, int log_buckets)
                : val(val), modulos(modulos), log_buckets(log_buckets), end(false) {}
            iterator(bool end) : end(end) {}
        public:
            iterator() : end(false) {}
            int operator*() const;
            iterator operator++();
            iterator operator++(int);
            bool operator==(iterator const &);
            bool operator!=(iterator const &);
        };
        iterator get_buckets(store_key_t);
        iterator end() { return iterator(true); }
        redundant_hasher_t(int log_redundancy) : log_buckets(0), log_redundancy(log_redundancy) {}
        void double_buckets() { log_buckets++; }
    };
    redundant_hasher_t rh;
    
public:
    typedef redundant_hasher_t::iterator rh_iterator; //for readability

    //add_storage_mailbox_t add_storage_mailbox;
    void add_storage(int peer_id, set_store_t *address) {
        inner_map[peer_id] = address;
        while (peer_id >= (1 << rh.log_buckets))
            rh.double_buckets();
    }

    //remove_storage_mailbox_t remove_storage_mailbox;
    void remove_storage(int peer_id) {
        inner_map.erase(peer_id);
    }

    class iterator {
    private:
        friend class storage_map_t;
        std::map<int, set_store_t*> *inner_map;
        redundant_hasher_t *rh;
        redundant_hasher_t::iterator hasher_iterator;
        iterator(std::map<int, set_store_t*> *inner_map, redundant_hasher_t *rh, redundant_hasher_t::iterator hasher_iterator);
    public:
        iterator() {}
        set_store_t *operator*() const;
        iterator operator++();
        iterator operator++(int);
        bool operator==(iterator const &);
        bool operator!=(iterator const &);
    };

    iterator get_storage(store_key_t key);
    iterator end();
    bool empty() { return inner_map.empty(); }

    storage_map_t()
        //: add_storage_mailbox(boost::bind(&storage_map_t::add_storage, this, _1, _2)),
          //remove_storage_mailbox(boost::bind(&storage_map_t::remove_storage, this, _1, _2))
        : rh(1)
    {}
};
/* The master map keeps track of how many write transactions are currently
 * pending on a given hash value, this is needed so that we can safely hand a
 * value off to another node */
/* private:
    class refcount_t {
    public:
        int val;
        multi_cond_t wait_for_0;
        refcount_t() : val(0) {}
        ~refcount_t() {
            guarantee(val == 0, "refcount_t destroyed with a refcount not equal to 0, this probably means that there's an outstanding sarc going one\n");
            wait_for_0.pulse();
        }
    };
    friend class outstanding_transaction_t;
    std::map<hash_t, int> refcounts;
    void incr_refcount(hash_t hash) {
        on_thread_t syncer(home_thread);
        guarantee(get_cluster().us == find(hash).second.peer, "Trying to relinquish a peer that isn't ours");
        refcounts[hash].val++;
    }
    void decr_refcount(hash_t hash) {
        on_thread_t syncer(home_thread);
        guarantee(get_cluster().us == std::map<hash_t, set_store_t>::find(hash).peer, "Trying to relinquish a peer that isn't ours");
        if (refcounts.find(hash) == refcounts.end() || refcounts[hash].val == 0)
            crash("Refcount decred below 0\n");
        refcounts[hash].val--;
        if (refcounts[hash].val == 0) refcounts.erase(hash);
    }
public:
    class outstanding_transaction_t {
        master_map_t *parent;
        outstanding_transaction_t(master_map_t *parent, hash_t hash) : parent(parent) {
            parent->incr_refcount(hash);
        }
        ~outstanding_transaction_t() {
            parent->decr_refcount(hash);
        }
    };

    void relinquish(hash_t hash, set_store_interface_mailbox_t::set_store_t::address_t new_master) {
        on_thread_t syncer(home_thread);
        guarantee(get_cluster().us == std::map<hash_t, set_store_t::address_t>::find(hash).peer, "Trying to relinquish a peer that isn't ours");
        if (refcounts.find(hash) != refcounts.end())
            refcounts[hash].wait_for_0.wait();
        std::map<hash_t, set_store_t::address_t>::find(hash) = new_master;
    }

}; */
#endif
