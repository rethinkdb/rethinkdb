// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/backfill.hpp"

#include "arch/runtime/coroutines.hpp"
#include "btree/depth_first_traversal.hpp"
#include "btree/leaf_node.hpp"
#include "concurrency/pmap.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"

/* `MAX_CONCURRENT_VALUE_LOADS` is the maximum number of coroutines we'll use for loading
values from the leaf nodes. */
static const int MAX_CONCURRENT_VALUE_LOADS = 16;

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(backfill_pre_item_t, range);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(backfill_item_t::pair_t, key, recency, value);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(backfill_item_t,
    range, pairs, min_deletion_timestamp);

void backfill_item_t::mask_in_place(const key_range_t &m) {
    range = range.intersection(m);
    std::vector<pair_t> new_pairs;
    for (auto &&pair : pairs) {
        if (m.contains_key(pair.key)) {
            new_pairs.push_back(std::move(pair));
        }
    }
    pairs = std::move(new_pairs);
}

/* `convert_to_right_bound()` is suitable for converting `btree_key_t *`s in the format
of `right_incl` or `left_excl_or_null` into a `key_range_t::right_bound_t`. */
key_range_t::right_bound_t convert_to_right_bound(const btree_key_t *rightmost_before) {
    if (rightmost_before == nullptr) {
        return key_range_t::right_bound_t(store_key_t::min());
    } else {
        key_range_t::right_bound_t rb;
        rb.unbounded = false;
        rb.key().assign(rightmost_before);
        bool ok = rb.increment();
        guarantee(ok);
        return rb;
    }
}

key_range_t convert_to_key_range(
        const btree_key_t *left_excl_or_null, const btree_key_t *right_incl) {
    return key_range_t(
        left_excl_or_null == nullptr
            ? key_range_t::bound_t::none : key_range_t::bound_t::open,
        left_excl_or_null,
        key_range_t::bound_t::closed,
        right_incl);
}

continue_bool_t btree_send_backfill_pre(
        superblock_t *superblock,
        release_superblock_t release_superblock,
        value_sizer_t *sizer,
        const key_range_t &range,
        repli_timestamp_t reference_timestamp,
        btree_backfill_pre_item_consumer_t *pre_item_consumer,
        signal_t *interruptor) {
    class callback_t : public depth_first_traversal_callback_t {
    public:
        continue_bool_t filter_range_ts(
                UNUSED const btree_key_t *left_excl_or_null,
                const btree_key_t *right_incl,
                repli_timestamp_t timestamp,
                signal_t *,
                bool *skip_out) {
            *skip_out = timestamp <= reference_timestamp;
            if (*skip_out) {
                return pre_item_consumer->on_empty_range(
                    convert_to_right_bound(right_incl));
            } else {
                return continue_bool_t::CONTINUE;
            }
        }

        continue_bool_t handle_pre_leaf(
                const counted_t<counted_buf_lock_and_read_t> &buf,
                const btree_key_t *left_excl_or_null,
                const btree_key_t *right_incl,
                signal_t *,
                bool *skip_out) {
            *skip_out = true;
            const leaf_node_t *lnode = static_cast<const leaf_node_t *>(
                buf->read->get_data_read());
            repli_timestamp_t min_deletion_timestamp =
                leaf::min_deletion_timestamp(sizer, lnode, buf->lock.get_recency());
            if (min_deletion_timestamp > reference_timestamp.next()) {
                /* We might be missing deletion entries, so re-transmit the entire node
                */
                backfill_pre_item_t pre_item;
                pre_item.range = convert_to_key_range(left_excl_or_null, right_incl);
                return pre_item_consumer->on_pre_item(std::move(pre_item));
            } else {
                std::vector<const btree_key_t *> keys;
                leaf::visit_entries(
                    sizer, lnode, buf->lock.get_recency(),
                    [&](const btree_key_t *key, repli_timestamp_t timestamp,
                            const void *) -> continue_bool_t {
                        if ((left_excl_or_null != nullptr &&
                                    btree_key_cmp(key, left_excl_or_null) <= 0)
                                || btree_key_cmp(key, right_incl) > 0) {
                            return continue_bool_t::CONTINUE;
                        }
                        /* `visit_entries()` calls the callback in order from newest to
                        oldest timestamps. So once we find one entry that's older than
                        the threshold we care about, we know that there won't be any more
                        entries that are newer than the threshold we care about, so we
                        abort. */
                        if (timestamp <= reference_timestamp) {
                            return continue_bool_t::ABORT;
                        }
                        keys.push_back(key);
                        return continue_bool_t::CONTINUE;
                    });
                std::sort(keys.begin(), keys.end(),
                    [](const btree_key_t *k1, const btree_key_t *k2) {
                        return btree_key_cmp(k1, k2) < 0;
                    });
                for (const btree_key_t *key : keys) {
                    backfill_pre_item_t pre_item;
                    pre_item.range = key_range_t(key);
                    if (continue_bool_t::ABORT ==
                            pre_item_consumer->on_pre_item(std::move(pre_item))) {
                        return continue_bool_t::ABORT;
                    }
                }
                return pre_item_consumer->on_empty_range(
                    convert_to_right_bound(right_incl));
            }
        }

        continue_bool_t handle_pair(scoped_key_value_t &&, signal_t *) {
            unreachable();
        }

        continue_bool_t handle_empty(
                UNUSED const btree_key_t *left_excl_or_null,
                const btree_key_t *right_incl,
                signal_t *) {
            return pre_item_consumer->on_empty_range(
                    convert_to_right_bound(right_incl));
        }

        btree_backfill_pre_item_consumer_t *pre_item_consumer;
        repli_timestamp_t reference_timestamp;
        value_sizer_t *sizer;
    } callback;
    callback.pre_item_consumer = pre_item_consumer;
    callback.reference_timestamp = reference_timestamp;
    callback.sizer = sizer;
    return btree_depth_first_traversal(superblock, range, &callback, access_t::read,
        FORWARD, release_superblock, interruptor);
}

/* The `backfill_item_loader_t` gets backfill items from the `backfill_item_preparer_t`,
but that the actual row values have not been loaded into the items yet. It loads the
values from the cache and then passes the items on to the `backfill_item_consumer_t`. */
class backfill_item_loader_t {
public:
    backfill_item_loader_t(
            btree_backfill_item_consumer_t *_item_consumer, cond_t *_abort_cond) :
        item_consumer(_item_consumer), abort_cond(_abort_cond),
        semaphore(MAX_CONCURRENT_VALUE_LOADS) { }

    /* `on_item()` and `on_empty_range()` will be called in lexicographical order. They
    will always be called from the same coroutine, so if a call blocks the traversal will
    be paused until it returns. */

    /* The item passed to `on_item` is complete except for the `value` field of each
    pair. If the pair has a value, then instead of containing that value, `pair->value`
    will contain a pointer into `buf_read.get_data_read()` which can be used to actually
    load the value. The pointer is stored in the `std::vector<char>` that would normally
    be supposed to store the value. This is kind of a hack, but it will work and it's
    localized to these two types. */
    void on_item(
            backfill_item_t &&item,
            const counted_t<counted_buf_lock_and_read_t> &buf,
            signal_t *interruptor) {
        new_semaphore_acq_t sem_acq(&semaphore, item.pairs.size());
        wait_interruptible(sem_acq.acquisition_signal(), interruptor);
        coro_t::spawn_sometime(std::bind(
            &backfill_item_loader_t::handle_item, this,
            std::move(item), buf, std::move(sem_acq),
            fifo_source.enter_write(), drainer.lock()));
    }

    void on_empty_range(
            const key_range_t::right_bound_t &empty_range, signal_t *interruptor) {
        new_semaphore_acq_t sem_acq(&semaphore, 1);
        wait_interruptible(sem_acq.acquisition_signal(), interruptor);
        /* Unlike `handle_item()`, `handle_empty_range()` doesn't do any expensive work,
        but we spawn it in a coroutine anyway so that it runs in the right order with
        respect to `handle_item()`. */
        coro_t::spawn_sometime(std::bind(
            &backfill_item_loader_t::handle_empty_range, this,
            empty_range, std::move(sem_acq), fifo_source.enter_write(), drainer.lock()));
    }

    void finish(signal_t *interruptor) {
        fifo_enforcer_sink_t::exit_write_t exit_write(
            &fifo_sink, fifo_source.enter_write());
        wait_interruptible(&exit_write, interruptor);
    }

private:
    void handle_item(
            backfill_item_t &item,
            const counted_t<counted_buf_lock_and_read_t> &buf,
            const new_semaphore_acq_t &,
            fifo_enforcer_write_token_t token,
            auto_drainer_t::lock_t keepalive) {
        try {
            pmap(item.pairs.size(), [&](size_t i) {
                try {
                    if (!static_cast<bool>(item.pairs[i].value)) {
                        /* It's a deletion; we don't need to load anything. */
                        return;
                    }
                    rassert(item.pairs[i].value->size() == sizeof(void *));
                    void *value_ptr = *reinterpret_cast<void *const *>(
                        item.pairs[i].value->data());
                    item.pairs[i].value->clear();
                    item_consumer->copy_value(buf_parent_t(&buf->lock), value_ptr,
                        keepalive.get_drain_signal(), &*item.pairs[i].value);
                } catch (const interrupted_exc_t &) {
                    /* we'll check this outside the `pmap()` */
                }
            });
            if (keepalive.get_drain_signal()->is_pulsed()) {
                throw interrupted_exc_t();
            }
            fifo_enforcer_sink_t::exit_write_t exit_write(&fifo_sink, token);
            wait_interruptible(&exit_write, keepalive.get_drain_signal());
            if (abort_cond->is_pulsed()) {
                return;
            }
            if (continue_bool_t::ABORT == item_consumer->on_item(std::move(item))) {
                abort_cond->pulse();
            }
        } catch (const interrupted_exc_t &) {
            /* ignore */
        }
    }

    void handle_empty_range(
            const key_range_t::right_bound_t &threshold,
            const new_semaphore_acq_t &,
            fifo_enforcer_write_token_t token,
            auto_drainer_t::lock_t keepalive) {
        try {
            fifo_enforcer_sink_t::exit_write_t exit_write(&fifo_sink, token);
            wait_interruptible(&exit_write, keepalive.get_drain_signal());
            if (abort_cond->is_pulsed()) {
                return;
            }
            if (continue_bool_t::ABORT == item_consumer->on_empty_range(threshold)) {
                abort_cond->pulse();
            }
        } catch (const interrupted_exc_t &) {
            /* ignore */
        }
    }

    btree_backfill_item_consumer_t *item_consumer;
    cond_t *abort_cond;
    new_semaphore_t semaphore;
    fifo_enforcer_source_t fifo_source;
    fifo_enforcer_sink_t fifo_sink;
    auto_drainer_t drainer;
};

/* `backfill_item_preparer_t` visits leaf nodes using callbacks from the
`btree_depth_first_traversal()`. At each leaf node, it constructs a series of
`backfill_item_t`s describing the leaf, but doesn't set their values yet; in place of the
values, it stores a pointer to where the value can be loaded from the leaf. Then it
passes them to the `backfill_item_loader_t` to do the actual loading. */
class backfill_item_preparer_t : public depth_first_traversal_callback_t {
public:
    backfill_item_preparer_t(
            value_sizer_t *_sizer,
            repli_timestamp_t _reference_timestamp,
            btree_backfill_pre_item_producer_t *_pre_item_producer,
            signal_t *_abort_cond,
            backfill_item_loader_t *_loader) :
        sizer(_sizer), reference_timestamp(_reference_timestamp),
        pre_item_producer(_pre_item_producer), abort_cond(_abort_cond), loader(_loader)
        { }

private:
    /* Skip B-tree subtrees that haven't changed since the reference timestamp and that
    don't overlap with any pre-items' ranges. */
    continue_bool_t filter_range_ts(
            const btree_key_t *left_excl_or_null,
            const btree_key_t *right_incl,
            repli_timestamp_t timestamp,
            signal_t *interruptor,
            bool *skip_out) {
        *skip_out = false;
        if (timestamp <= reference_timestamp) {
            key_range_t range = convert_to_key_range(left_excl_or_null, right_incl);
            if (pre_item_producer->try_consume_empty_range(range)) {
                *skip_out = true;
                loader->on_empty_range(range.right, interruptor);
            }
        }
        return abort_cond->is_pulsed()
            ? continue_bool_t::ABORT : continue_bool_t::CONTINUE;
    }

    continue_bool_t handle_pre_leaf(
            const counted_t<counted_buf_lock_and_read_t> &buf,
            const btree_key_t *left_excl_or_null,
            const btree_key_t *right_incl,
            signal_t *interruptor,
            bool *skip_out) {
        *skip_out = true;
        key_range_t::right_bound_t cursor = convert_to_right_bound(left_excl_or_null);
        key_range_t::right_bound_t right_bound = convert_to_right_bound(right_incl);
        while (cursor != right_bound) {
            key_range_t subrange;
            subrange.left = cursor.key();
            std::list<backfill_item_t> items_from_pre;
            continue_bool_t cont = pre_item_producer->consume_range(
                &cursor, right_bound,
                [&](const backfill_pre_item_t &pre_item) {
                    items_from_pre.push_back(backfill_item_t());
                    items_from_pre.back().range = pre_item.range;
                });
            if (cont == continue_bool_t::ABORT) {
                return continue_bool_t::ABORT;
            }
            subrange.right = cursor;
            rassert(!subrange.is_empty());
            handle_pre_leaf_subrange(
                buf, subrange, std::move(items_from_pre), interruptor);
            if (abort_cond->is_pulsed()) {
                return continue_bool_t::ABORT;
            }
        }
        return continue_bool_t::CONTINUE;
    }

    /* `handle_pre_leaf_subrange()` creates backfill items for the given subrange of the
    given leaf, for which the pre-items are already available. The pre-items are
    delivered in the form of a `std::list<backfill_item_t>` where each item's range is
    the same as one of the pre-items, but the other fields of the item are uninitialized.
    */
    void handle_pre_leaf_subrange(
            const counted_t<counted_buf_lock_and_read_t> &buf,
            const key_range_t &subrange,
            std::list<backfill_item_t> &&items_from_pre,
            signal_t *interruptor) {
        const leaf_node_t *lnode = static_cast<const leaf_node_t *>(
            buf->read->get_data_read());
        repli_timestamp_t min_deletion_timestamp =
            leaf::min_deletion_timestamp(sizer, lnode, buf->lock.get_recency());
        if (min_deletion_timestamp > reference_timestamp) {
            /* We might be missing deletion entries, so re-transmit the entire node as a
            single `backfill_item_t` */
            backfill_item_t item;
            item.min_deletion_timestamp = min_deletion_timestamp;
            item.range = subrange;

            /* Create a `backfill_item_t::pair_t` for each key-value pair in the leaf
            node, but don't load the values yet */
            leaf::visit_entries(
                sizer, lnode, buf->lock.get_recency(),
                [&](const btree_key_t *key, repli_timestamp_t timestamp,
                        const void *value_or_null) -> continue_bool_t {
                    /* Ignore keys from the leaf node that aren't within our sub-range */
                    if (!subrange.contains_key(key)) {
                        return continue_bool_t::CONTINUE;
                    }

                    size_t i = item.pairs.size();
                    item.pairs.resize(i + 1);
                    item.pairs[i].key.assign(key);
                    item.pairs[i].recency = timestamp;
                    if (value_or_null != nullptr) {
                        /* Store `value_or_null` in the `value` field as a sequence of
                        8 (or 4 or whatever) `char`s describing its actual pointer value.
                        */
                        item.pairs[i].value = std::vector<char>(
                            reinterpret_cast<const char *>(&value_or_null),
                            reinterpret_cast<const char *>(1 + &value_or_null));
                    }
                    return continue_bool_t::CONTINUE;
                });

            /* `visit_entries()` returns entries in timestamp order; we need to sort
            `item.pairs` now to put it in lexicographical order */
            std::sort(item.pairs.begin(), item.pairs.end(),
                [](const backfill_item_t::pair_t &p1,
                        const backfill_item_t::pair_t &p2) {
                    return p1.key < p2.key;
                });

            /* Note that `on_item()` may block, which will limit the rate at which we
            traverse the B-tree. */
            loader->on_item(std::move(item), buf, interruptor);

        } else {
            /* Attach `min_deletion_timestamp` to `items_from_pre`, because it hasn't
            been initialized yet for them. We also need to clip `items_from_pre` because
            some of them might fall partially outside of `subrange`. */
            for (backfill_item_t &i : items_from_pre) {
                rassert(subrange.overlaps(i.range));
                i.range = i.range.intersection(subrange);
                i.min_deletion_timestamp = min_deletion_timestamp;
            }

            /* Find each key-value pair or deletion entry that falls within the range of
            a pre item or that changed since `reference_timestamp`. If it falls within
            the range of a pre item, put it into the corresponding item in
            `items_from_pre`; otherwise, make a new item for it in `items_from_time`. */
            std::list<backfill_item_t> items_from_time;
            leaf::visit_entries(
                sizer, lnode, buf->lock.get_recency(),
                [&](const btree_key_t *key, repli_timestamp_t timestamp,
                        const void *value_or_null) -> continue_bool_t {
                    /* The leaf node might be partially outside the range of the
                    backfill, so we might have to skip some keys */
                    if (!subrange.contains_key(key)) {
                        return continue_bool_t::CONTINUE;
                    }

                    /* In the most common case, `items_from_pre` is totally empty. Since
                    we ignore entries that are at `reference_timestamp` or older unless
                    they are in a pre item, we can optimize things slightly by aborting
                    the leaf node iteration early. */
                    if (timestamp <= reference_timestamp && items_from_pre.empty()) {
                        return continue_bool_t::ABORT;
                    }

                    /* We'll set `item` to the `backfill_item_t` where this key-value
                    pair should be inserted. First we check if there's an item in
                    `items_from_pre` that contains the key. If not, we'll create a new
                    item in `items_from_time`. */
                    backfill_item_t *item = nullptr;
                    /* Linear search sucks, but there probably won't be a whole lot of
                    pre items, so it's OK for now. */
                    for (backfill_item_t &a : items_from_pre) {
                        if (a.range.contains_key(key)) {
                            item = &a;
                            break;
                        }
                    }
                    if (item == nullptr) {
                        /* We didn't find an item in `items_from_pre`. */
                        if (timestamp > reference_timestamp) {
                            /* We should create a new item for this key-value pair */
                            items_from_time.push_back(backfill_item_t());
                            item = &items_from_time.back();
                            item->range = key_range_t(key);
                            item->min_deletion_timestamp =
                                repli_timestamp_t::distant_past;
                        } else {
                            /* Ignore this key-value pair */
                            return continue_bool_t::CONTINUE;
                        }
                    }

                    rassert(item->range.contains_key(key));

                    /* This is `timestamp.next()` instead of `timestamp` because it's
                    possible for us to forget a deletion with timestamp equal to the
                    oldest timestamped entry, so `min_deletion_timestamp` is one time
                    unit newer than the oldest timestamped entry. */
                    rassert(timestamp.next() >= item->min_deletion_timestamp);

                    size_t i = item->pairs.size();
                    item->pairs.resize(i + 1);
                    item->pairs[i].key.assign(key);
                    item->pairs[i].recency = timestamp;
                    if (value_or_null != nullptr) {
                        /* Store `value_or_null` in the `value` field as a sequence of
                        8 (or 4 or whatever) `char`s describing its actual pointer value.
                        */
                        item->pairs[i].value = std::vector<char>(
                            reinterpret_cast<const char *>(&value_or_null),
                            reinterpret_cast<const char *>(1 + &value_or_null));
                    }
                    return continue_bool_t::CONTINUE;
                });

            /* `leaf::visit_entries` returns entries in timestamp order, but we want
            `items_from_time` to be in lexicographical order, so we need to sort it. Same
            with the `pairs` field of each item in `items_from_pre`. */
            items_from_time.sort(
                [](const backfill_item_t &i1, const backfill_item_t &i2) {
                    return i1.range.left < i2.range.left;
                });
            for (backfill_item_t &i : items_from_pre) {
                std::sort(i.pairs.begin(), i.pairs.end(),
                    [](const backfill_item_t::pair_t &p1,
                            const backfill_item_t::pair_t &p2) {
                        return p1.key < p2.key;
                    });
            }

            /* Merge `items_from_time` into `items_from_pre`, preserving order. */
            items_from_pre.merge(
                std::move(items_from_time),
                [](const backfill_item_t &i1, const backfill_item_t &i2) {
                    rassert(!i1.range.overlaps(i2.range));
                    return i1.range.left < i2.range.left;
                });

            /* Send the results to the loader */
            for (backfill_item_t &i : items_from_pre) {
                loader->on_item(std::move(i), buf, interruptor);
            }
            loader->on_empty_range(subrange.right, interruptor);
        }
    }

    continue_bool_t handle_pair(scoped_key_value_t &&, signal_t *) {
        unreachable();
    }

    continue_bool_t handle_empty(
            const btree_key_t *left_excl_or_null,
            const btree_key_t *right_incl,
            signal_t *interruptor) {
        key_range_t::right_bound_t cursor = convert_to_right_bound(left_excl_or_null);
        key_range_t::right_bound_t end = convert_to_right_bound(right_incl);
        while (cursor != end) {
            std::vector<backfill_item_t> items;
            continue_bool_t cont = pre_item_producer->consume_range(
                &cursor, end,
                [&](const backfill_pre_item_t &pre_item) {
                    items.resize(items.size() + 1);
                    items.back().range = pre_item.range;
                    items.back().min_deletion_timestamp = repli_timestamp_t::distant_past;
                });
            if (cont == continue_bool_t::ABORT) {
                return continue_bool_t::ABORT;
            }
            for (backfill_item_t &i : items) {
                loader->on_item(
                    std::move(i), counted_t<counted_buf_lock_and_read_t>(), interruptor);
            }
            loader->on_empty_range(cursor, interruptor);
        }
        return abort_cond->is_pulsed()
            ? continue_bool_t::ABORT : continue_bool_t::CONTINUE;
    }

    value_sizer_t *sizer;
    repli_timestamp_t reference_timestamp;
    btree_backfill_pre_item_producer_t *pre_item_producer;
    signal_t *abort_cond;
    backfill_item_loader_t *loader;
};

continue_bool_t btree_send_backfill(
        superblock_t *superblock,
        release_superblock_t release_superblock,
        value_sizer_t *sizer,
        const key_range_t &range,
        repli_timestamp_t reference_timestamp,
        btree_backfill_pre_item_producer_t *pre_item_producer,
        btree_backfill_item_consumer_t *item_consumer,
        signal_t *interruptor) {
    cond_t abort_cond;
    backfill_item_loader_t loader(item_consumer, &abort_cond);
    backfill_item_preparer_t preparer(
        sizer, reference_timestamp, pre_item_producer, &abort_cond, &loader);
    continue_bool_t cont = btree_depth_first_traversal(
            superblock, range, &preparer, access_t::read, FORWARD, release_superblock,
            interruptor);
    /* Wait for `loader` to finish what it's doing, even if `pre_item_producer` aborted.
    This is important so that we can make progress even if `pre_item_producer` only gives
    us a couple of pre-items at a time. */
    loader.finish(interruptor);
    return abort_cond.is_pulsed() ? continue_bool_t::ABORT : cont;
}

class backfill_deletion_timestamp_updater_t : public depth_first_traversal_callback_t {
public:
    backfill_deletion_timestamp_updater_t(value_sizer_t *s, repli_timestamp_t mdt) :
        sizer(s), min_deletion_timestamp(mdt) { }
    continue_bool_t handle_pre_leaf(
            const counted_t<counted_buf_lock_and_read_t> &buf,
            UNUSED const btree_key_t *left_excl_or_null,
            UNUSED const btree_key_t *right_incl,
            signal_t *,
            bool *skip_out) {
        *skip_out = true;
        buf_write_t buf_write(&buf->lock);
        leaf_node_t *lnode = static_cast<leaf_node_t *>(buf_write.get_data_write());
        leaf::erase_deletions(sizer, lnode, min_deletion_timestamp);
        return continue_bool_t::CONTINUE;
    }
    continue_bool_t handle_pair(scoped_key_value_t &&, signal_t *) {
        unreachable();
    }
private:
    value_sizer_t *sizer;
    repli_timestamp_t min_deletion_timestamp;
};

void btree_receive_backfill_item_update_deletion_timestamps(
        superblock_t *superblock,
        release_superblock_t release_superblock,
        value_sizer_t *sizer,
        const backfill_item_t &item,
        signal_t *interruptor) {
    backfill_deletion_timestamp_updater_t updater(sizer, item.min_deletion_timestamp);
    continue_bool_t res = btree_depth_first_traversal(
        superblock, item.range, &updater, access_t::write, FORWARD, release_superblock,
        interruptor);
    guarantee(res == continue_bool_t::CONTINUE);
}

