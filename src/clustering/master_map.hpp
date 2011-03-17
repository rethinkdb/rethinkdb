#ifndef _CLUSTERING_MASTER_MAP_HPP_
#define _CLUSTERING_MASTER_MAP_HPP_
#include <map>
#include "rpc/rpc.hpp"
#include "concurrency/cond_var.hpp"
#include <boost/bind.hpp>
#include "store.hpp"
#include "rpc/serialize/serialize.hpp"
#include "clustering/cluster_store.hpp"

typedef unsigned int hash_t;

//typedef set_store_interface_mailbox_t::address_t store_mailbox_t;
//typedef store_mailbox_t::address_t set_store_t;
//
typedef sync_mailbox_t<void(int, set_store_mailbox_t::address_t)> set_master_mailbox_t;
//typedef sync_mailbox_t<void(hash_t, set_store_t)> take_master_mailbox_t;
//
//typedef sync_mailbox_t<void(hash_t, set_store_t)> add_storage_mailbox_t;
//typedef sync_mailbox_t<void(hash_t, set_store_t)> remove_storage_mailbox_t;
//
//typedef sync_mailbox_t<void()> move_choke_mailbox_t;
//
//typedef sync_mailbox_t<void()> downgrade_redundancy_mailbox_t;

/* TODO, this could be implemented using drain_semaphores. The one thing is
 * that they're not reusable (so we would need to destroy and realloc them) */
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
};

/* hasher_t represents a hash function, right now it's pretty useless but
 * eventually we will need to use these to make sure that we get uncorrelated
 * hash functions */
class hasher_t {
public:
    hasher_t() {}
    hash_t hash(store_key_t k) { 
        unsigned int res = 0;
        for (int i = 0; i < k.size; i++) {
            res ^= res << 18;
            res += res >> 7;
            res += k.contents[i];
        }
        return res;
    }
};

/* the master_map keeps track of who is the master for a given key */
class master_map_t :
    public home_thread_mixin_t
{
public:
    /* set_store_txn_ts are created when a person ask for the master for a key
     * (with get_master). This is a way of enforcing that we know how many
     * outstanding transactions there are */
    class set_store_txn_t :
        public set_store_t
    {
    public:
        mutation_result_t change(const mutation_t& mut, castime_t cas) { return store->change(mut, cas); }
        set_store_txn_t() : store(NULL), parent(NULL), bucket(-1) {}
        set_store_txn_t(const set_store_txn_t &other) 
            : store(other.store), parent(other.parent), bucket(other.bucket) 
        { if (bucket != -1) parent->open_txn(bucket); }
    private:
        friend class master_map_t;

        set_store_txn_t(set_store_t *store, master_map_t *parent, int bucket)
            : store(store), parent(parent), bucket(bucket) 
        { parent->open_txn(bucket); }

        ~set_store_txn_t() { if (bucket != -1) parent->close_txn(bucket); }

    private:
        set_store_t *store;
        master_map_t *parent;
        int bucket;
    };

public:
    /* this is the only public facing part of the master map (along with the
     * class this returns) */
    set_store_txn_t get_master(store_key_t);

private:
    std::map<int, set_store_t*> inner_map;

    class master_hasher_t {
    public:
        int log_buckets;
        hasher_t hasher;
    public:
        int get_bucket(store_key_t);
        master_hasher_t() : log_buckets(0) {}
        void double_buckets() { log_buckets++; }
    };
    master_hasher_t master_hasher;

/* The master map is responsible for managing and communicating with the other
 * peers about responsibility changes */
private:
    set_master_mailbox_t set_master_mailbox;
    void set_master(int bucket, set_store_mailbox_t::address_t address) {
        drain(bucket); //make sure there are no outstanding writes the the store
        inner_map[bucket] = &address;
    }

private:
    /* the refcounts map is used to keep track of how many outstanding
     * transactions there are for a master, this is needed for when we hand
     * them off */
    friend class set_store_txn_t;
    refcount_map_t<int> refcounts;
    void open_txn(int bucket) { 
        on_thread_t syncer(home_thread);
        refcounts.incr_refcount(bucket); 
    }
    void close_txn(int bucket) { 
        on_thread_t syncer(home_thread);
        refcounts.decr_refcount(bucket); 
    }

private:
    class bucket_block_t {
    public:
        multi_cond_t *cond;
        bucket_block_t() : cond(new multi_cond_t()) {}
        ~bucket_block_t() {
            cond->pulse();
            delete cond;
        }
    };
    std::map<int, bucket_block_t> bucket_blocks;
    /* drain a bucket of all current transactions, returns when the bucket is
     * drained */
    void drain(int bucket) {
        on_thread_t syncer(home_thread);
        bucket_blocks[bucket] = bucket_block_t();
        refcounts.wait(bucket);
    }
    /* opens a bucket back up */
    void un_block(int bucket) {
        on_thread_t syncer(home_thread);
        bucket_blocks.erase(bucket);
    }

    void wait(int bucket) {
        if (bucket_blocks.find(bucket) == bucket_blocks.end()) return;
        else bucket_blocks[bucket].cond->wait();
    }

public:
    master_map_t()
        : set_master_mailbox(boost::bind(&master_map_t::set_master, this, _1, _2))
    {}
};

/* the storage_map keeps track of who is storing certain keys */
class set_store_t;
class get_store_t;
typedef std::map<int, std::pair<set_store_t*, get_store_t*> > internal_storage_map_t;
class storage_map_t {
private:
    internal_storage_map_t inner_map;

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
    void add_storage(int peer_id, std::pair<set_store_t *, get_store_t *> addr_pair) {
        inner_map[peer_id] = addr_pair;
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
        internal_storage_map_t *inner_map;
        redundant_hasher_t *rh;
        redundant_hasher_t::iterator hasher_iterator;
        iterator(internal_storage_map_t *inner_map, redundant_hasher_t *rh, redundant_hasher_t::iterator hasher_iterator);
    public:
        iterator() {}
        std::pair<set_store_t *, get_store_t *>operator*() const;
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
#endif
