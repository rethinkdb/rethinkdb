#include "rdb_protocol/store.hpp"

#include "btree/backfill.hpp"
#include "btree/reql_specific.hpp"
#include "btree/operations.hpp"
#include "rdb_protocol/blob_wrapper.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/lazy_json.hpp"

/* After every `MAX_BACKFILL_ITEMS_PER_TXN` backfill items or backfill pre-items, we'll
release the superblock and start a new transaction. */
static const int MAX_BACKFILL_ITEMS_PER_TXN = 100;

/* `limiting_btree_backfill_pre_item_consumer_t` accepts `backfill_pre_item_t`s from
`btree_send_backfill_pre()` and forwards them to the given
`store_view_t::backfill_pre_item_consumer_t`, but it aborts after it receives a certain
number of them. The purpose of this is to avoid holding the B-tree superblock for too
long. */
class limiting_btree_backfill_pre_item_consumer_t :
    public btree_backfill_pre_item_consumer_t
{
public:
    limiting_btree_backfill_pre_item_consumer_t(
            store_view_t::backfill_pre_item_consumer_t *_inner,
            key_range_t::right_bound_t *_threshold_ptr) :
        inner_aborted(false), inner(_inner), remaining(MAX_BACKFILL_ITEMS_PER_TXN),
        threshold_ptr(_threshold_ptr) { }
    continue_bool_t on_pre_item(backfill_pre_item_t &&item)
            THROWS_NOTHING {
        --remaining;
        rassert(key_range_t::right_bound_t(item.range.left) >=
            *threshold_ptr);
        *threshold_ptr = item.range.right;
        inner_aborted =
            continue_bool_t::ABORT == inner->on_pre_item(std::move(item));
        return (inner_aborted || remaining == 0)
            ? continue_bool_t::ABORT : continue_bool_t::CONTINUE;
    }
    continue_bool_t on_empty_range(
            const key_range_t::right_bound_t &new_threshold) THROWS_NOTHING {
        --remaining;
        rassert(new_threshold >= *threshold_ptr);
        *threshold_ptr = new_threshold;
        inner_aborted =
            continue_bool_t::ABORT == inner->on_empty_range(new_threshold);
        return (inner_aborted || remaining == 0)
            ? continue_bool_t::ABORT : continue_bool_t::CONTINUE;
    }
    bool inner_aborted;
private:
    store_view_t::backfill_pre_item_consumer_t *inner;
    size_t remaining;
    key_range_t::right_bound_t *threshold_ptr;
};

continue_bool_t store_t::send_backfill_pre(
        const region_map_t<state_timestamp_t> &start_point,
        backfill_pre_item_consumer_t *pre_item_consumer,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    /* `start_point` is in the form of a `region_map_t`, so we might have different
    start timestamps for different regions. But `btree_send_backfill_pre()` expects a
    single homogeneous timestamp. So we have to do each sub-region of `start_point`
    individually. */
    std::vector<std::pair<key_range_t, repli_timestamp_t> > reference_timestamps;
    for (const auto &pair : start_point) {
        guarantee(pair.first.beg == get_region().beg
            && pair.first.end == get_region().end,
            "start_point should be homogeneous with respect to hash shard because this "
            "implementation ignores hashes");
        reference_timestamps.push_back(std::make_pair(
            pair.first.inner, pair.second.to_repli_timestamp()));
    }
    /* Sort the sub-regions so we can apply them from left to right */
    std::sort(reference_timestamps.begin(), reference_timestamps.end(),
        [](const std::pair<key_range_t, repli_timestamp_t> &p1,
                const std::pair<key_range_t, repli_timestamp_t> &p2) -> bool {
            guarantee(!p1.first.overlaps(p2.first));
            return p1.first.left < p2.first.left;
        });
    for (const auto &pair : reference_timestamps) {
        /* Within each sub-region, we may make multiple separate B-tree transactions.
        This is to avoid holding the B-tree superblock for too long at once. */
        key_range_t::right_bound_t threshold(pair.first.left);
        while (threshold != pair.first.right) {
            scoped_ptr_t<txn_t> txn;
            scoped_ptr_t<real_superblock_t> sb;
            get_btree_superblock_and_txn_for_reading(
                general_cache_conn.get(), CACHE_SNAPSHOTTED_NO, &sb, &txn);

            limiting_btree_backfill_pre_item_consumer_t
                limiter(pre_item_consumer, &threshold);

            rdb_value_sizer_t sizer(cache->max_block_size());
            key_range_t to_do = pair.first;
            to_do.left = threshold.key();
            continue_bool_t cont = btree_send_backfill_pre(sb.get(),
                release_superblock_t::RELEASE, &sizer, to_do, pair.second, &limiter,
                interruptor);
            guarantee(threshold <= pair.first.right);
            if (limiter.inner_aborted) {
                guarantee(cont == continue_bool_t::ABORT);
                return continue_bool_t::ABORT;
            }
            guarantee(cont == continue_bool_t::ABORT || threshold == pair.first.right);
        }
    }
    return continue_bool_t::CONTINUE;
}

/* `pre_item_buffer_t` takes pre-items from `store_view_t::backfill_pre_item_producer_t`
and makes them available to `btree_send_backfill()` via its `producer_t` member type,
which subclasses from `btree_backfill_pre_item_producer_t`. It supports rewinding, in the
sense that it's possible to consume the same range of pre-items multiple times; it does
this by keeping the pre-items in an internal buffer instead of discarding them as the
`producer_t` uses them. Call `discard()` to actually remove pre-items from the internal
buffer. */
class pre_item_buffer_t {
public:
    /* A single `producer_t` must proceed from left to right over the key-space,
    consuming every pre-item exactly once. But it's OK to destroy the `producer_t` and
    create another one for the same `pre_item_buffer_t`, then use it to consume the same
    range of pre-items again. */
    class producer_t : public btree_backfill_pre_item_producer_t {
    public:
        /* `_threshold` is where this producer will start reading from. */
        producer_t(pre_item_buffer_t *_parent,
                const key_range_t::right_bound_t &_threshold) :
            parent(_parent), threshold(_threshold) {
            guarantee(threshold >= parent->left);
            guarantee(!parent->has_child);
            parent->has_child = true;
        }
        ~producer_t() {
            guarantee(parent->has_child);
            parent->has_child = false;
            parent->buffer_past.splice(parent->buffer_past.end(), parent->buffer_future);
            std::swap(parent->buffer_past, parent->buffer_future);
        }
        continue_bool_t consume_range(
                const btree_key_t *left_excl_or_null,
                const btree_key_t *right_incl,
                const std::function<void(const backfill_pre_item_t &)> &callback) {
            key_range_t range(
                left_excl_or_null == nullptr
                    ? key_range_t::bound_t::none : key_range_t::bound_t::open,
                left_excl_or_null,
                key_range_t::bound_t::closed,
                right_incl);
            guarantee(key_range_t::right_bound_t(range.left) == threshold);
            threshold = range.right;
            while (parent->right < range.right) {
                if (continue_bool_t::ABORT == parent->fetch_more()) {
                    return continue_bool_t::ABORT;
                }
            }
            while (true) {
                if (!parent->buffer_future.empty() && key_range_t::right_bound_t(
                        parent->buffer_future.front().range.left) < range.right) {
                    callback(parent->buffer_future.front());
                } else {
                    break;
                }
                if (parent->buffer_future.front().range.right <= range.right) {
                    parent->buffer_past.splice(parent->buffer_past.end(),
                        parent->buffer_future, parent->buffer_future.begin());
                } else {
                    break;
                }
            }
            return continue_bool_t::CONTINUE;
        }
        continue_bool_t peek_range(
                const btree_key_t *left_excl_or_null,
                const btree_key_t *right_incl,
                bool *has_pre_items_out) {
            key_range_t range(
                left_excl_or_null == nullptr
                    ? key_range_t::bound_t::none : key_range_t::bound_t::open,
                left_excl_or_null,
                key_range_t::bound_t::closed,
                right_incl);
            guarantee(key_range_t::right_bound_t(range.left) == threshold);
            if (!parent->buffer_future.empty() && key_range_t::right_bound_t(
                    parent->buffer_future.front().range.left) < range.right) {
                *has_pre_items_out = true;
                return continue_bool_t::CONTINUE;
            } else if (parent->right >= range.right) {
                *has_pre_items_out = false;
                return continue_bool_t::CONTINUE;
            } else {
                /* We don't have enough information to determine if there is a pre item
                in the range. We could fetch pre items into `buffer_future` until we knew
                for sure. But if `next_pre_item()` returned `ABORT`, then we'd have to
                abort; and since `peek_range()` is often used to look far into the
                future, that's potentially very inefficient. So we do the conservative
                thing and say that there are pre items in the range even though there
                might not be.

                Maybe it would be better if we called `next_pre_item()` until it ran out
                of pre items, and then decide based on those results. But that would
                require changing the semantics of `next_pre_item()` to allow us to call
                it again after it returns `ABORT`, so it's not worth it unless this turns
                out to be a bottleneck. */
                *has_pre_items_out = true;
                return continue_bool_t::CONTINUE;
            }
        }
    private:
        pre_item_buffer_t *parent;
        /* `threshold` is the point we've consumed up to. It proceeds monotonically to
        the right. */
        key_range_t::right_bound_t threshold;
    };
    pre_item_buffer_t(store_view_t::backfill_pre_item_producer_t *_inner,
            const key_range_t::right_bound_t &_left) :
        inner(_inner), left(_left), right(left), has_child(false) { }
    void discard(const key_range_t::right_bound_t &bound) {
        guarantee(bound >= left);
        guarantee(bound <= right);
        left = bound;
        while (!buffer_past.empty() && buffer_past.front().range.right <= bound) {
            buffer_past.pop_front();
        }
        guarantee(buffer_future.empty() || buffer_future.front().range.right > bound);
    }
private:
    /* `producer_t` calls `fetch_more()` when there aren't enough pre-items in the
    buffer. */
    continue_bool_t fetch_more() {
        bool is_item;
        backfill_pre_item_t const *item;
        key_range_t::right_bound_t empty_range;
        if (continue_bool_t::ABORT ==
                inner->next_pre_item(&is_item, &item, &empty_range)) {
            return continue_bool_t::ABORT;
        }
        if (is_item) {
            rassert(key_range_t::right_bound_t(item->range.left) >= right);
            buffer_future.push_back(*item);
            inner->release_pre_item();
            right = item->range.right;
        } else {
            right = empty_range;
        }
        return continue_bool_t::CONTINUE;
    }
    store_view_t::backfill_pre_item_producer_t *inner;

    /* `right` indicates the point we've read up to; `left` indicates the point we've
    discarded up to. `buffer_past` and `buffer_future` together contain all the pre-items
    between `left` and `right`, possibly including a pre-item that spans `left`.
    `buffer_past` contains the pre-items that the current `producer_t` has already
    consumed, and `buffer_future` contains the pre-items that are yet to be consumed.
    If there is no active `producer_t` then all the pre-items go in `buffer_future`. */
    key_range_t::right_bound_t left, right;
    std::list<backfill_pre_item_t> buffer_past, buffer_future;

    /* `true` if a `producer_t` exists. This is used to make sure we never have multiple
    `producer_t`s at the same time. */
    bool has_child;
};

/* `limiting_btree_backfill_item_consumer_t` is like the `..._pre_item_consumer_t` type
defined earlier in this file, except for items instead of pre-items. It also takes care
of handling metainfo and discarding pre-items from the `pre_item_buffer_t` when they
aren't needed anymore. */
class limiting_btree_backfill_item_consumer_t :
    public btree_backfill_item_consumer_t {
public:
    limiting_btree_backfill_item_consumer_t(
            store_view_t::backfill_item_consumer_t *_inner,
            key_range_t::right_bound_t *_threshold_ptr,
            pre_item_buffer_t *_pre_item_buffer,
            const region_map_t<binary_blob_t> *_metainfo_ptr) :
        inner_aborted(false), inner(_inner), remaining(MAX_BACKFILL_ITEMS_PER_TXN),
        threshold_ptr(_threshold_ptr), pre_item_buffer(_pre_item_buffer),
        metainfo_ptr(_metainfo_ptr) { }
    continue_bool_t on_item(backfill_item_t &&item) {
        --remaining;
        rassert(key_range_t::right_bound_t(item.range.left) >=
            *threshold_ptr);
        *threshold_ptr = item.range.right;

        /* We'll never need to traverse the range to the left of `item.range.right`
        again, so it's safe to trim `pre_item_buffer` up to that point to save space in
        memory. */
        pre_item_buffer->discard(item.range.right);

        inner_aborted = continue_bool_t::ABORT == inner->on_item(
            *metainfo_ptr, std::move(item));
        return (inner_aborted || remaining == 0)
            ? continue_bool_t::ABORT : continue_bool_t::CONTINUE;
    }
    continue_bool_t on_empty_range(
            const key_range_t::right_bound_t &new_threshold) {
        --remaining;
        rassert(new_threshold >= *threshold_ptr);
        *threshold_ptr = new_threshold;

        pre_item_buffer->discard(new_threshold);

        inner_aborted = continue_bool_t::ABORT == inner->on_empty_range(
            *metainfo_ptr, new_threshold);
        return (inner_aborted || remaining == 0)
            ? continue_bool_t::ABORT : continue_bool_t::CONTINUE;
    }
    void copy_value(
            buf_parent_t parent,
            const void *value_in_leaf_node,
            UNUSED signal_t *interruptor2,
            std::vector<char> *value_out) {
        const rdb_value_t *v =
            static_cast<const rdb_value_t *>(value_in_leaf_node);
        rdb_blob_wrapper_t blob_wrapper(
            parent.cache()->max_block_size(),
            const_cast<rdb_value_t *>(v)->value_ref(),
            blob::btree_maxreflen);
        blob_acq_t acq_group;
        buffer_group_t buffer_group;
        blob_wrapper.expose_all(
            parent, access_t::read, &buffer_group, &acq_group);
        value_out->resize(buffer_group.get_size());
        size_t offset = 0;
        for (size_t i = 0; i < buffer_group.num_buffers(); ++i) {
            buffer_group_t::buffer_t b = buffer_group.get_buffer(i);
            memcpy(value_out->data() + offset, b.data, b.size);
            offset += b.size;
        }
        guarantee(offset == value_out->size());
    }
    bool inner_aborted;
private:
    store_view_t::backfill_item_consumer_t *const inner;
    size_t remaining;
    key_range_t::right_bound_t *const threshold_ptr;

    /* We hold a pointer to the `pre_item_buffer_t` so we can call `discard()` on it. */
    pre_item_buffer_t *const pre_item_buffer;

    /* `metainfo_ptr` points to the metainfo that applies to the items we're handling.
    Note that it can't be changed. This is OK because `limiting_..._consumer_t` never
    exists across multiple B-tree transactions, so the metainfo is constant. */
    const region_map_t<binary_blob_t> *const metainfo_ptr;
};

continue_bool_t store_t::send_backfill(
        const region_map_t<state_timestamp_t> &start_point,
        backfill_pre_item_producer_t *pre_item_producer,
        backfill_item_consumer_t *item_consumer,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    /* Just like in `send_backfill_pre()`, we first break `start_point` up into regions
    with homogeneous start timestamps, then backfill each region as a series of multiple
    B-tree transactions to avoid holding the superblock too long. */
    std::vector<std::pair<key_range_t, repli_timestamp_t> > reference_timestamps;
    for (const auto &pair : start_point) {
        guarantee(pair.first.beg == get_region().beg
            && pair.first.end == get_region().end,
            "start_point should be homogeneous with respect to hash shard because this "
            "implementation ignores hashes");
        reference_timestamps.push_back(std::make_pair(
            pair.first.inner, pair.second.to_repli_timestamp()));
    }
    std::sort(reference_timestamps.begin(), reference_timestamps.end(),
        [](const std::pair<key_range_t, repli_timestamp_t> &p1,
                const std::pair<key_range_t, repli_timestamp_t> &p2) -> bool {
            guarantee(!p1.first.overlaps(p2.first));
            return p1.first.left < p2.first.left;
        });
    pre_item_buffer_t pre_item_buffer(
        pre_item_producer,
        key_range_t::right_bound_t(start_point.get_domain().inner.left));
    for (const auto &pair : reference_timestamps) {
        key_range_t::right_bound_t threshold(pair.first.left);
        while (threshold != pair.first.right) {
            scoped_ptr_t<txn_t> txn;
            scoped_ptr_t<real_superblock_t> sb;
            get_btree_superblock_and_txn_for_reading(
                general_cache_conn.get(), CACHE_SNAPSHOTTED_NO, &sb, &txn);
            region_map_t<binary_blob_t> metainfo;
            get_metainfo_internal(sb.get(), &metainfo);

            pre_item_buffer_t::producer_t buffered_producer(
                &pre_item_buffer, threshold);
            limiting_btree_backfill_item_consumer_t limiter(
                item_consumer, &threshold, &pre_item_buffer, &metainfo);

            rdb_value_sizer_t sizer(cache->max_block_size());
            key_range_t to_do = pair.first;
            to_do.left = threshold.key();
            continue_bool_t cont = btree_send_backfill(sb.get(),
                release_superblock_t::RELEASE, &sizer, to_do, pair.second,
                &buffered_producer, &limiter, interruptor);
            if (limiter.inner_aborted) {
                guarantee(cont == continue_bool_t::ABORT);
                return continue_bool_t::ABORT;
            }
            guarantee(cont == continue_bool_t::ABORT || threshold == pair.first.right);
        }
    }
    return continue_bool_t::CONTINUE;
}

