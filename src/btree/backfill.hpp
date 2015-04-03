// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_BACKFILL_HPP_
#define BTREE_BACKFILL_HPP_

#include <map>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "btree/keys.hpp"
#include "btree/types.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/interruptor.hpp"
#include "repli_timestamp.hpp"
#include "rpc/serialize_macros.hpp"

class buf_parent_t;
class superblock_t;
class value_sizer_t;

class backfill_pre_atom_t {
public:
    key_range_t get_range() const {
        return range;
    }
    size_t get_mem_size() const {
        return sizeof(backfill_pre_atom_t);
    }
    void mask_in_place(const key_range_t &m) {
        range = range.intersection(m);
    }
    key_range_t range;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_pre_atom_t);

class backfill_atom_t {
public:
    class pair_t {
    public:
        store_key_t key;
        repli_timestamp_t recency;
        boost::optional<std::vector<char> > value;   /* empty indicates deletion */
    };
    key_range_t get_range() const {
        return range;
    }
    size_t get_mem_size() const {
        size_t s = sizeof(backfill_atom_t);
        for (const auto &pair : pairs) {
            s += sizeof(pair_t);
            if (static_cast<bool>(pair.value)) {
                s += pair.value->size();
            }
        }
        return s;
    }
    void mask_in_place(const key_range_t &m);
    key_range_t range;
    repli_timestamp_t deletion_cutoff_timestamp;
    std::vector<pair_t> pairs;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_atom_t::pair_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_atom_t);

class btree_backfill_pre_atom_consumer_t {
public:
    virtual continue_bool_t on_pre_atom(backfill_pre_atom_t &&atom) THROWS_NOTHING = 0;
    virtual continue_bool_t on_empty_range(const key_range_t::right_bound_t &threshold)
        THROWS_NOTHING = 0;
private:
    virtual ~btree_backfill_pre_atom_consumer_t() { }
};

continue_bool_t btree_backfill_pre_atoms(
    superblock_t *superblock,
    release_superblock_t release_superblock,
    const value_sizer_t *sizer,
    const key_range_t &range,
    repli_timestamp_t since_when,
    btree_backfill_pre_atom_consumer_t *pre_atom_consumer,
    signal_t *interruptor);

class btree_backfill_pre_atom_producer_t {
public:
    virtual continue_bool_t peek_range(
        const btree_key_t *left_excl_or_null, const btree_key_t *right_incl,
        bool *has_pre_atoms_out) = 0;
    virtual continue_bool_t consume_range(
        const btree_key_t *left_excl_or_null, const btree_key_t *right_incl,
        const std::function<void(const backfill_pre_atom_t *)> &callback) = 0;
protected:
    virtual ~btree_backfill_pre_atom_producer_t() { }
};

class btree_backfill_atom_consumer_t {
public:
    virtual continue_bool_t on_atom(backfill_atom_t &&atom) = 0;
    virtual continue_bool_t on_empty_range(const key_range_t::right_bound_t &threshold) = 0;
    virtual void copy_value(
        buf_parent_t buf_parent,
        const void *value_in_leaf_node,
        signal_t *interruptor,
        std::vector<char> *value_out) = 0;
public:
    virtual ~btree_backfill_atom_consumer_t() { }
};

continue_bool_t btree_backfill_atoms(
    superblock_t *superblock,
    release_superblock_t release_superblock,
    const value_sizer_t *sizer,
    const key_range_t &range,
    repli_timestamp_t since_when,
    btree_backfill_pre_atom_producer_t *pre_atom_producer,
    btree_backfill_atom_consumer_t *atom_consumer,
    signal_t *interruptor);

#endif  // BTREE_BACKFILL_HPP_

