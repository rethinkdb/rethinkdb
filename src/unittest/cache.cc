#include <stdlib.h>
#include <string.h>

#include "arch/runtime/coroutines.hpp"
#include "arch/timing.hpp"
#include "buffer_cache/alt/page.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "concurrency/auto_drainer.hpp"
#include "serializer/config.hpp"
#include "unittest/gtest.hpp"
#include "unittest/mock_file.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

using alt::alt_cache_t;
using alt::alt_txn_t;
using alt::alt_buf_lock_t;
using alt::alt_buf_parent_t;
using alt::alt_create_t;
using alt::alt_access_t;
using alt::alt_buf_read_t;
using alt::alt_buf_write_t;

// RSI this is copy pasted from unittest/page_test.cc put it somewhere general
struct mock_ser_t {
    mock_file_opener_t opener;
    scoped_ptr_t<standard_serializer_t> ser;

    mock_ser_t()
        : opener() {
        standard_serializer_t::create(&opener,
                                      standard_serializer_t::static_config_t());
        ser = make_scoped<standard_serializer_t>(log_serializer_t::dynamic_config_t(),
                                                 &opener,
                                                 &get_global_perfmon_collection());
    }
};

void runCacheCreateDestroy() {
    mock_ser_t mock;
    alt_cache_t cache(mock.ser.get());
}

TEST(Cache, DISABLED_CreateDestroy) {
    run_in_thread_pool(runCacheCreateDestroy, 4);
}

void runMakeTxns() {
    mock_ser_t mock;
    alt_cache_t cache(mock.ser.get());

    alt_txn_t wtxn1(&cache, write_durability_t::HARD);
    alt_txn_t wtxn2(&cache, write_durability_t::SOFT);

    alt_txn_t rtxn1(&cache);
    alt_txn_t rtxn2(&cache);
}

void delete_txn(alt_txn_t *txn) {
    delete txn;
}

void runTxnPredecessors() {
    mock_ser_t mock;
    alt_cache_t cache(mock.ser.get());

    auto wtxn1 = new alt_txn_t(&cache, write_durability_t::HARD);
    coro_t::spawn(std::bind(&delete_txn, wtxn1));
    alt_txn_t wtxn2(&cache, write_durability_t::SOFT, wtxn1);

    auto rtxn1 = new alt_txn_t(&cache);
    coro_t::spawn(std::bind(&delete_txn, rtxn1));
    alt_txn_t rtxn2(&cache, rtxn1);
}

/* This test is disabled because it hangs, I'm not sure why yet. */
TEST(Cache, DISABLED_TxnPredecessors) {
    run_in_thread_pool(runTxnPredecessors, 4);
}

class block_grinder_t {
public:
    block_grinder_t() : cache(mock.ser.get()) { }
    alt_buf_lock_t read(block_id_t i, alt_buf_parent_t p) {
        guarantee(std_contains(in_memory_record, i));
        alt_buf_lock_t lock(p, i, alt_access_t::read);
        {
            alt_buf_read_t data(&lock);
            EXPECT_EQ(in_memory_record[i], *static_cast<const int *>(data.get_data_read()));
        }
        return lock;
    }
    alt_buf_lock_t write(block_id_t i,  alt_buf_parent_t p) {
        if (std_contains(in_memory_record, i)) {
            alt_buf_lock_t lock(p, i, alt_access_t::write);
            {
                alt_buf_write_t data(&lock);
                int *val = static_cast<int *>(data.get_data_write());
                EXPECT_EQ(in_memory_record[i], *val);
                *val = ++in_memory_record[i];
            }
            return lock;
        } else {
            alt_buf_lock_t lock(p, i, alt_create_t::create);
            {
                alt_buf_write_t data(&lock);
                int *val = static_cast<int *>(data.get_data_write());
                *val = in_memory_record[i] = 0;
            }
            return lock;
        }
    }

    mock_ser_t mock;
    alt_cache_t cache;
    std::map<block_id_t, int> in_memory_record;
};

/* A block thread represents a parented thread of block aquisitions. */
class block_thread_t {
public:
    /* Creates a new read txn. */
    block_thread_t(block_grinder_t *_parent)
        : parent(_parent), txn(new alt_txn_t(&parent->cache)),
          first_acq(true) { }

    /* Creates a new write txn. */
    block_thread_t(block_grinder_t *_parent, write_durability_t durability)
        : parent(_parent),
          txn(new alt_txn_t(&parent->cache, durability)),
          first_acq(true) { }

    /* Spins off a new thread of acquisition from an acquisition from an
     * existing one. */
    block_thread_t(block_thread_t *sibling)
        : parent(sibling->parent), txn(sibling->txn) { }

    void read(block_id_t bid) {
        if (first_acq) {
            first_acq = false;
            buf_lock = parent->read(bid, alt_buf_parent_t(txn.get()));
        } else {
            buf_lock = parent->read(bid, alt_buf_parent_t(&buf_lock));
        }
    }
    void write(block_id_t bid) {
        if (first_acq) {
            first_acq = false;
            buf_lock = parent->write(bid, alt_buf_parent_t(txn.get()));
        } else {
            buf_lock = parent->write(bid, alt_buf_parent_t(&buf_lock));
        }
    }
    void snapshot() {
        buf_lock.snapshot_subtree();
    }

    block_grinder_t *parent;
    boost::shared_ptr<alt_txn_t> txn;
    alt_buf_lock_t buf_lock;
    bool first_acq;
};

void runSingleBlock() {
    block_grinder_t grinder;
    {
        block_thread_t thread(&grinder, write_durability_t::HARD);
        thread.write(1);
    }
    {
        block_thread_t thread(&grinder);
        thread.read(1);
    }
}

TEST(Cache, SingleBlock) {
    run_in_thread_pool(runSingleBlock, 4);
}

void write_chain(block_grinder_t *g, block_id_t start,
        block_id_t end, auto_drainer_t::lock_t) {
    block_thread_t thread(g, write_durability_t::HARD);
    for (block_id_t i = start; i < end; ++i) {
        thread.write(i);
        coro_t::yield();
    }
}

void read_chain(block_grinder_t *g, block_id_t start,
        block_id_t end, auto_drainer_t::lock_t) {
    block_thread_t thread(g);
    for (block_id_t i = start; i < end; ++i) {
        thread.read(i);
        coro_t::yield();
    }
}

void runBlockChain() {
    block_grinder_t grinder;
    auto_drainer_t drainer;
    for (int i = 0; i < 100; ++i) {
        coro_t::spawn(std::bind(&write_chain, &grinder, 0, 100, drainer.lock()));
        coro_t::spawn(std::bind(&read_chain, &grinder, 0, 100, drainer.lock()));
    }
}

TEST(Cache, BlockChain) {
    run_in_thread_pool(runBlockChain, 4);
}

/* Think of these tree like a heapified array. It's a binary tree where the
 * node with id `n` has 2 children 2n and 2n + 1. */
block_id_t next_random_tree_node(block_id_t id) {
    return (id * 2) + randint(2);
}

int tree_levels = 4;

void init_tree(block_grinder_t *g) {
    block_thread_t thread(g, write_durability_t::HARD);
    for (block_id_t id = 1; id < 1 << tree_levels; ++id) {
        thread.write(id);
    }
}

void write_tree(block_grinder_t *g, auto_drainer_t::lock_t) {
    block_thread_t thread(g, write_durability_t::HARD);
    for (block_id_t id = 1; id < (1 << tree_levels); id = next_random_tree_node(id)) {
        thread.write(id);
        coro_t::yield();
    }
}

void read_tree(block_grinder_t *g, auto_drainer_t::lock_t) {
    block_thread_t thread(g);
    for (block_id_t id = 1; id < (1 << tree_levels); id = next_random_tree_node(id)) {
        thread.read(id);
        coro_t::yield();
    }
}

void runBlockTree() {
    block_grinder_t grinder;
    init_tree(&grinder);

    auto_drainer_t drainer;
    for (int i = 0; i < 100; ++i) {
        coro_t::spawn(std::bind(&write_tree, &grinder, drainer.lock()));
        coro_t::spawn(std::bind(&read_tree, &grinder, drainer.lock()));
    }
}

TEST(Cache, BlockTree) {
    run_in_thread_pool(runBlockTree, 4);
}

void runSnapshotTest() {
    block_grinder_t grinder;
    init_tree(&grinder);

    block_thread_t read_thread(&grinder);
    block_id_t read_id = 1;
    read_thread.read(read_id);
    read_id = next_random_tree_node(read_id);

    auto_drainer_t drainer;
    for (int i = 0; i < 100; ++i) {
        coro_t::spawn(std::bind(&write_tree, &grinder, drainer.lock()));
    }
}

/* This test hangs right now, turn it back on when snapshotting works. */
TEST(Cache, DISABLED_Snapshot) {
    run_in_thread_pool(runSnapshotTest, 4);
}

void init_tree_and_chains(block_grinder_t *g) {
    block_thread_t thread(g, write_durability_t::HARD);
    for (block_id_t id = 1; id < 1 << tree_levels; ++id) {
        thread.write(id);
        auto_drainer_t dummy;
        write_chain(g, (id + (1 << tree_levels)) * 100,
                (id + (1 << tree_levels) + 1) * 100, dummy.lock());
    }
}

void write_tree_maybe_chain(block_grinder_t *g, auto_drainer_t::lock_t) {
    block_thread_t thread(g, write_durability_t::HARD);
    for (block_id_t id = 1; id < (1 << tree_levels); id = next_random_tree_node(id)) {
        thread.write(id);
        if (randint(2) == 0) {
            auto_drainer_t drainer;
            for (int i = 0; i < 5; ++i) {
                coro_t::spawn(std::bind(&write_chain, g,
                            (id + (1 << tree_levels)) * 100,
                            (id + (1 << tree_levels) + 1) * 100, drainer.lock()));
            }
        }
        coro_t::yield();
    }
}

void read_tree_maybe_chain(block_grinder_t *g, auto_drainer_t::lock_t) {
    block_thread_t thread(g, write_durability_t::HARD);
    for (block_id_t id = 1; id < (1 << tree_levels); id = next_random_tree_node(id)) {
        thread.write(id);
        if (randint(2) == 0) {
            auto_drainer_t drainer;
            for (int i = 0; i < 5; ++i) {
                coro_t::spawn(std::bind(&read_chain, g,
                            (id + (1 << tree_levels)) * 100,
                            (id + (1 << tree_levels) + 1) * 100, drainer.lock()));
            }
        }
        coro_t::yield();
    }
}

void runTreeAndChains() {
    block_grinder_t grinder;
    init_tree_and_chains(&grinder);

    auto_drainer_t drainer;

    for (int i = 0; i < 40; ++i) {
        coro_t::spawn(std::bind(&write_tree_maybe_chain, &grinder, drainer.lock()));
        coro_t::spawn(std::bind(&read_tree_maybe_chain, &grinder, drainer.lock()));
    }
}

/* This test hangs right now, not sure if it's a bug in the test or in the new
 * cache. */
TEST(Cache, DISABLED_TreeAndChains) {
    run_in_thread_pool(runTreeAndChains, 4);
}

} //namespace unittest 
