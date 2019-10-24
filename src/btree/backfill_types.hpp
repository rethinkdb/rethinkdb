#ifndef BTREE_BACKFILL_TYPES_HPP_
#define BTREE_BACKFILL_TYPES_HPP_

#include <vector>

#include "btree/keys.hpp"
#include "containers/optional.hpp"
#include "repli_timestamp.hpp"
#include "rpc/serialize_macros.hpp"

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
        optional<std::vector<char> > value;   /* empty indicates deletion */
        size_t get_mem_size() const {
            size_t s = sizeof(pair_t);
            if (static_cast<bool>(value)) {
                s += value->size();
            }
            return s;
        }
        /* The size of a `pair_t` assuming its value is set to the given size */
        static size_t get_mem_size_with_value(size_t value_size) {
            return sizeof(pair_t) + value_size;
        }
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
            s += pair.get_mem_size();
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

#endif  // BTREE_BACKFILL_TYPES_HPP_
