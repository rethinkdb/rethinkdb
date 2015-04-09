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

class backfill_pre_item_t {
public:
    key_range_t get_range() const {
        return range;
    }
    size_t get_mem_size() const {
        return sizeof(backfill_pre_item_t);
    }
    void mask_in_place(const key_range_t &m) {
        range = range.intersection(m);
    }
    key_range_t range;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_pre_item_t);

/* A `backfill_item_t` is a complete description of some sub-range of the B-tree,
containing all the information necessary to bring the corresponding sub-range of any
other B-tree into a state equivalent to that of the B-tree that the `backfill_item_t`
came from. The `range` member describes what range of keys it applies to. `pairs`
contains an entry for every key present in the range. In addition, for every key that has
been deleted from the range with timestamp greater than or equal to
`min_deletion_timestamp`, `pairs` will contain an deletion entry. Every pair carries a
timestamp, which is greater than or equal to the timestamp of the actual query that last
touched the key. */
class backfill_item_t {
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
        size_t s = sizeof(backfill_item_t);
        for (const auto &pair : pairs) {
            s += sizeof(pair_t);
            if (static_cast<bool>(pair.value)) {
                s += pair.value->size();
            }
        }
        return s;
    }
    void mask_in_place(const key_range_t &m);
    bool is_single_key() {
        key_range_t::right_bound_t x(range.left);
        x.increment();
        return pairs.size() == 1 && x == range.right;
    }
    key_range_t range;
    std::vector<pair_t> pairs;
    repli_timestamp_t min_deletion_timestamp;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_item_t::pair_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_item_t);

class btree_backfill_pre_item_consumer_t {
public:
    virtual continue_bool_t on_pre_item(backfill_pre_item_t &&item) THROWS_NOTHING = 0;
    virtual continue_bool_t on_empty_range(const key_range_t::right_bound_t &threshold)
        THROWS_NOTHING = 0;
protected:
    virtual ~btree_backfill_pre_item_consumer_t() { }
};

continue_bool_t btree_send_backfill_pre(
    superblock_t *superblock,
    release_superblock_t release_superblock,
    value_sizer_t *sizer,
    const key_range_t &range,
    repli_timestamp_t reference_timestamp,
    btree_backfill_pre_item_consumer_t *pre_item_consumer,
    signal_t *interruptor);

/* `btree_backfill_pre_item_producer_t` is responsible for feeding a stream of
`backfill_pre_item_t`s to `btree_backfill_items()`. */
class btree_backfill_pre_item_producer_t {
public:
    /* `consume_range()` will be called on a series of contiguous ranges in
    lexicographical order. For each range, it must find all the `backfill_pre_item_t`s
    that overlap that range and call the callback with them in lexicographical order. If
    a `backfill_pre_item_t` is partially inside and partially outside the range,
    `consume_range()` may pass the entire thing to the callback. */
    virtual continue_bool_t consume_range(
        const btree_key_t *left_excl_or_null, const btree_key_t *right_incl,
        const std::function<void(const backfill_pre_item_t &)> &callback) = 0;

    /* `peek_range()` will always be called with `left_excl_or_null` equal to the
    starting point of the next call to `consume_range()`. It may be called zero or more
    times between each pair of calls to `consume_range()`, possibly with different values
    of `right_incl`. It should set `*has_pre_items_out` to `true` if at least one pre
    item overlaps the range and `false` otherwise. It's OK for it to incorrectly set
    `*has_pre_items_out` to `true` even if there are no pre items, although this may
    reduce performance. */
    virtual continue_bool_t peek_range(
        const btree_key_t *left_excl_or_null, const btree_key_t *right_incl,
        bool *has_pre_items_out) = 0;
protected:
    virtual ~btree_backfill_pre_item_producer_t() { }
};

class btree_backfill_item_consumer_t {
public:
    virtual continue_bool_t on_item(
        backfill_item_t &&item) = 0;
    virtual continue_bool_t on_empty_range(
        const key_range_t::right_bound_t &threshold) = 0;
    virtual void copy_value(
        buf_parent_t buf_parent,
        const void *value_in_leaf_node,
        signal_t *interruptor,
        std::vector<char> *value_out) = 0;
public:
    virtual ~btree_backfill_item_consumer_t() { }
};

continue_bool_t btree_send_backfill(
    superblock_t *superblock,
    release_superblock_t release_superblock,
    value_sizer_t *sizer,
    const key_range_t &range,
    repli_timestamp_t reference_timestamp,
    btree_backfill_pre_item_producer_t *pre_item_producer,
    btree_backfill_item_consumer_t *item_consumer,
    signal_t *interruptor);

/* Erases any deletion entries or timestamps in leaf nodes within `item.range` if the
timestamp is earlier than `item.min_deletion_timestamp`. Note that this does NOT erase
any key-value pairs, nor does it apply any key-value pairs from `item` to the B-tree. The
caller must do those separately. */
void btree_receive_backfill_item_update_deletion_timestamps(
    superblock_t *superblock,
    release_superblock_t release_superblock,
    value_sizer_t *sizer,
    const backfill_item_t &item,
    signal_t *interruptor);

#endif  // BTREE_BACKFILL_HPP_

