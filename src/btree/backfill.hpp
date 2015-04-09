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

/* `backfill_pre_item_t` describes a range of keys which have changed on the backfill
destination since the source and destination diverged. The backfill destination sends
pre items to the backfill source so that the source knows to re-transmit the values of
those keys. */
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
came from. */
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

    /* `get_mem_size()` estimates the size the `backfill_item_t` occupies in RAM or when
    serialized over the network. Backfill batch sizes are defined in terms of the total
    mem sizes of all the backfill items in the batch. */
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

    /* A single-key backfill item consists of a `key_range_t` covering exactly one key,
    and a single `pair_t` for that same key. The backfill receiver logic has a separate
    code path for single-key items in order to improve performance. */
    bool is_single_key() {
        key_range_t::right_bound_t x(range.left);
        x.increment();
        return pairs.size() == 1 && x == range.right;
    }

    /* The `range` member describes what range of keys this item applies to. `pairs`
    contains an entry for every key present in the range on the backfill source. In
    addition, for every key that has been deleted from the range with timestamp greater
    than or equal to `min_deletion_timestamp`, `pairs` will contain an deletion entry.
    Every pair carries a timestamp, which is greater than or equal to the timestamp of
    the actual query that last touched the key. */
    key_range_t range;
    std::vector<pair_t> pairs;
    repli_timestamp_t min_deletion_timestamp;

    /* TODO: For single-key items, this is not very memory-efficient, because we store
    the key in three places: in `pairs[0].key`, in `range.left`, and in `range.right.key`
    minus one. Consider wrapping `range` in a `scoped_ptr_t`, which is left empty for a
    single-key item. This will also reduce data copying when moving an item. */
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_item_t::pair_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_item_t);

/* `btree_send_backfill_pre()` finds all of the keys or ranges of keys that have changed
since `reference_timestamp` and describes them as a sequence of `backfill_pre_item_t`s,
which it passes to `pre_item_consumer->on_pre_item()`. The pre-items will not overlap,
and the calls to `on_pre_item()` are guaranteed to go in lexicographical order from left
to right. `on_empty_range()` indicates that there won't be any pre items between the end
of the last pre item and the threshold passed to `on_empty_range()`.

If `on_pre_item()` or `on_empty_range()` returns `ABORT`, then no more calls will be
generated and `btree_send_backfill_pre()` will return `ABORT`. Otherwise,
`btree_send_backfill_pre()` will continue until it reaches the end of `range` and then
return `CONTINUE`. The final call to `on_pre_item()` or `on_empty_range()` is guaranteed
to end exactly on `range.right`. */

class btree_backfill_pre_item_consumer_t {
public:
    /* These callbacks may block, but `btree_send_backfill_pre()` might hold B-tree locks
    while it calls them, so they shouldn't block for long. */
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
`backfill_pre_item_t`s to `btree_send_backfill()`. */
class btree_backfill_pre_item_producer_t {
public:
    /* These callbacks may block, but `btree_send_backfill()` may hold B-tree locks while
    it calls them, so they shouldn't block for long. */

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

/* `btree_send_backfill()` finds all of the keys or key-ranges which have changed since
`reference_timestamp` or which are included in one of the `btree_pre_item_t`s provided
by `pre_item_producer`. It generates a stream of `backfill_item_t`s which describe those
keys and their associated values and timestamps. It passes the `backfill_item_t`s to the
given `btree_backfill_item_consumer_t`. */

class btree_backfill_item_consumer_t {
public:
    /* The exact semantics for how `btree_send_backfill()` calls `on_item()` and
    `on_empty_range()` are the same as for `btree_send_pre_backfill()` and
    `btree_backfill_item_consumer_t`. */
    virtual continue_bool_t on_item(
        backfill_item_t &&item) = 0;
    virtual continue_bool_t on_empty_range(
        const key_range_t::right_bound_t &threshold) = 0;

    /* `copy_value()` is responsible for reading a value out of the leaf node, finding
    the corresponding blob pages (if it's a large value), and putting the contents into
    `value_out`. This is because the exact format of the leaf node values is opaque to
    the B-tree logic. `copy_value()` may block. */
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

/* There's no such thing as `btree_receive_backfill()`; the RDB protocol code is
responsible for interpreting the `backfill_item_t`s and translating them into a series
of deletions, insertions, etc. to be applied to the B-tree.

`btree_receive_backfill_item_update_deletion_timestamps()` (what a mouthful!) is a helper
function that the RDB protocol code calls when applying `backfill_item_t`s. When a
`backfill_item_t` only contains deletions going back to some `min_deletion_timestamp`, we
must record in the B-tree that deletions before that point have been forgotten. We do
this by erasing timestamps and deletion entries before that point in the leaf node. This
function visits every leaf node in `item.range` and erases any timestamps or deletion
entries with timestamps earlier than `item.min_deletion_timestamp`. It should be called
for every `backfill_item_t` that's applied to the B-tree, unless `item.is_single_key()`
returns `true`. */
void btree_receive_backfill_item_update_deletion_timestamps(
    superblock_t *superblock,
    release_superblock_t release_superblock,
    value_sizer_t *sizer,
    const backfill_item_t &item,
    signal_t *interruptor);

#endif  // BTREE_BACKFILL_HPP_

