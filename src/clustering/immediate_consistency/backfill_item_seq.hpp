#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_ITEM_SEQ_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_ITEM_SEQ_HPP_

#include "rdb_protocol/protocol.hpp"

/* A `backfill_item_seq_t` contains all of the `backfill_{pre_}item_t`s in some range of
the key-space. The items are stored in lexicographical order.

It keeps track of the left and right bounds of the range it's supposed to apply to. For
example, if a `backfill_item_seq_t` contains items with keys `A`, `G`, and `J` and
applies to the range from `A` to `M`, that implies that there won't ever be a backfill
item at e.g. `B` or `L`, but it doesn't imply anything about whether there will be a
backfill item at `X`.

It also keeps a running total of the combined mem size of the backfill items it contains.
*/

template<class item_t>
class backfill_item_seq_t {
public:
    /* Initializes an undefined seq that cannot be used for anything. */
    backfill_item_seq_t() { }

    /* Initializes an empty seq with a zero-width region at the given location */
    backfill_item_seq_t(
            uint64_t _beg_hash, uint64_t _end_hash, key_range_t::right_bound_t key) :
        beg_hash(_beg_hash), end_hash(_end_hash), left_key(key), right_key(key),
        mem_size(0) { }

    /* Returns the left and right bounds of the space this `backfill_item_seq_t` applies
    to. These may be equal, in which case the sequence must be empty. */
    key_range_t::right_bound_t get_left_key() const { return left_key; }
    key_range_t::right_bound_t get_right_key() const { return right_key; }

    /* The `backfill_item_seq_t` is associated with a hash range for sanity-checking
    purposes. */
    uint64_t get_beg_hash() const { return beg_hash; }
    uint64_t get_end_hash() const { return end_hash; }

    region_t get_region() const {
        if (left_key == right_key) {
            return region_t::empty();
        } else {
            key_range_t kr;
            kr.left = left_key.key();
            kr.right = right_key;
            return region_t(beg_hash, end_hash, kr);
        }
    }

    size_t get_mem_size() const { return mem_size; }

    typename std::list<item_t>::const_iterator begin() const { return items.begin(); }
    typename std::list<item_t>::const_iterator end() const { return items.end(); }

    /* `empty_of_items()` returns `true` if there aren't any items in the seq; but the
    seq's domain might still be non-empty. `empty_domain()` returns `true` if the seq's
    domain is empty. `empty_domain()` implies `empty_of_items()`. */
    bool empty_of_items() const { return items.empty(); }
    bool empty_domain() const { return left_key == right_key; }

    const item_t &front() const { return items.front(); }
    const item_t &back() const { return items.back(); }

    /* Deletes the leftmost item in the seq, shrinking the seq's domain to after the end
    of the item that was removed. */
    void pop_front() {
        left_key = items.front().get_range().right;
        mem_size -= items.front().get_mem_size();
        items.pop_front();
    }

    /* Transfers the item at the {left,right} end of this seq to the {right,left} end of
    the other seq, shrinking the seq's domain to {after the end,before the beginning} of
    the item that was removed. */
    void pop_front_into(backfill_item_seq_t *other) {
        guarantee(beg_hash == other->beg_hash && end_hash == other->end_hash);
        guarantee(get_left_key() == other->get_right_key());
        size_t item_size = items.front().get_mem_size();
        left_key = items.front().get_range().right;
        other->right_key = left_key;
        mem_size -= item_size;
        other->mem_size += item_size;
        other->items.splice(other->items.end(), items, items.begin());
    }

    void pop_back_into(backfill_item_seq_t *other) {
        guarantee(beg_hash == other->beg_hash && end_hash == other->end_hash);
        guarantee(get_right_key() == other->get_left_key());
        size_t item_size = items.back().get_mem_size();
        right_key = key_range_t::right_bound_t(items.front().get_range().left);
        other->left_key = right_key;
        mem_size -= item_size;
        other->mem_size += item_size;
        other->items.splice(other->items.begin(), items, --items.end());
    }

    /* Deletes the part of the seq that is to the left of the key, changing the seq's
    left bound to the given location. If a single backfill item spans the key, that item
    will be split. */
    void delete_to_key(const key_range_t::right_bound_t &cut) {
        guarantee(cut >= get_left_key());
        guarantee(cut <= get_right_key());
        while (!items.empty()) {
            key_range_t range = items.front().get_range();
            if (range.right <= cut) {
                mem_size -= items.front().get_mem_size();
                items.pop_front();
            } else if (key_range_t::right_bound_t(range.left) >= cut) {
                break;
            } else {
                range.left = cut.key();
                mem_size -= items.front().get_mem_size();
                items.front().mask_in_place(range);
                mem_size += items.front().get_mem_size();
                break;
            }
        }
        left_key = cut;
    }

    /* Appends an item to the end of the seq, expanding the seq's domain on the right.
    Atoms must be appended in lexicographical order, so calling `push_back()` implicitly
    states that there are no items between the previous end of the seq and `item`. */
    void push_back(item_t &&item) {
        key_range_t item_range = item.get_range();
        guarantee(key_range_t::right_bound_t(item_range.left) >= right_key);
        right_key = item_range.right;
        mem_size += item.get_mem_size();
        items.push_back(std::move(item));
    }

    /* Expands the seq's domain on the right to the given key, explicitly stating that
    there are no items between the previous end of the seq and `bound`. */
    void push_back_nothing(const key_range_t::right_bound_t &bound) {
        guarantee(bound >= right_key);
        right_key = bound;
    }

    /* Expands the seq's domain on the left to the given key, explicitly stating that
    there are no items between the previous beginning of the seq and `bound`. */
    void push_front_nothing(const key_range_t::right_bound_t &bound) {
        guarantee(bound <= left_key);
        left_key = bound;
    }

    /* Concatenates two `backfill_item_seq_t`s. Their domains must be adjacent. */
    void concat(backfill_item_seq_t &&other) {
        guarantee(beg_hash == other.beg_hash && end_hash == other.end_hash);
        guarantee(right_key == other.left_key);
        right_key = other.right_key;
        mem_size += other.mem_size;
        items.splice(items.end(), std::move(other.items));
    }

private:
    /* A `backfill_item_seq_t` has a `region_t`, with one oddity: even when the region
    is empty, the `backfill_item_seq_t` still has a meaningful left and right bound. This
    is why this is stored as four separate variables instead of a `region_t`. We use two
    `key_range_t::right_bound_t`s instead of a `key_range_t` so that we can represent
    a zero-width region after the last key. */
    uint64_t beg_hash, end_hash;
    key_range_t::right_bound_t left_key, right_key;

    /* The sum of `a.get_mem_size()` over all the items */
    size_t mem_size;

    std::list<item_t> items;

    RDB_MAKE_ME_SERIALIZABLE_6(backfill_item_seq_t,
        beg_hash, end_hash, left_key, right_key, mem_size, items);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILL_ITEM_SEQ_HPP_ */

