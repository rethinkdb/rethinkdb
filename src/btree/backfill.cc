// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/backfill.hpp"

#include "arch/runtime/coroutines.hpp"
#include "btree/depth_first_traversal.hpp"
#include "btree/leaf_node.hpp"
#include "concurrency/pmap.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(backfill_pre_atom_t, range);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(backfill_atom_t::pair_t, key, recency, value);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(backfill_atom_t,
    range, pairs, min_deletion_timestamp);

void backfill_atom_t::mask_in_place(const key_range_t &m) {
    range = range.intersection(m);
    std::vector<pair_t> new_pairs;
    for (auto &&pair : pairs) {
        if (m.contains_key(pair.key)) {
            new_pairs.push_back(std::move(pair));
        }
    }
    pairs = std::move(new_pairs);
}

key_range_t::right_bound_t convert_right_bound(const btree_key_t *right_incl) {
    key_range_t::right_bound_t rb;
    rb.unbounded = false;
    rb.key().assign(right_incl);
    bool ok = rb.increment();
    guarantee(ok);
    return rb;
}

continue_bool_t btree_send_backfill_pre(
        superblock_t *superblock,
        release_superblock_t release_superblock,
        value_sizer_t *sizer,
        const key_range_t &range,
        repli_timestamp_t reference_timestamp,
        btree_backfill_pre_atom_consumer_t *pre_atom_consumer,
        signal_t *interruptor) {
    class callback_t : public depth_first_traversal_callback_t {
    public:
        continue_bool_t filter_range_ts(
                UNUSED const btree_key_t *left_excl_or_null,
                const btree_key_t *right_incl,
                repli_timestamp_t timestamp,
                bool *skip_out) {
            *skip_out = timestamp <= reference_timestamp;
            if (*skip_out) {
                return pre_atom_consumer->on_empty_range(
                    convert_right_bound(right_incl));
            } else {
                return continue_bool_t::CONTINUE;
            }
        }

        continue_bool_t handle_pre_leaf(
                const counted_t<counted_buf_lock_and_read_t> &buf,
                const btree_key_t *left_excl_or_null,
                const btree_key_t *right_incl,
                bool *skip_out) {
            *skip_out = true;
            const leaf_node_t *lnode = static_cast<const leaf_node_t *>(
                buf->read->get_data_read());
            repli_timestamp_t min_deletion_timestamp =
                leaf::min_deletion_timestamp(sizer, lnode, buf->lock.get_recency());
            if (min_deletion_timestamp > reference_timestamp.next()) {
                /* We might be missing deletion entries, so re-transmit the entire node
                */
                backfill_pre_atom_t pre_atom;
                pre_atom.range = key_range_t(
                    left_excl_or_null == nullptr
                        ? key_range_t::bound_t::none : key_range_t::bound_t::open,
                    left_excl_or_null,
                    key_range_t::bound_t::closed,
                    right_incl);
                return pre_atom_consumer->on_pre_atom(std::move(pre_atom));
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
                    backfill_pre_atom_t pre_atom;
                    pre_atom.range = key_range_t(key);
                    if (continue_bool_t::ABORT ==
                            pre_atom_consumer->on_pre_atom(std::move(pre_atom))) {
                        return continue_bool_t::ABORT;
                    }
                }
                return pre_atom_consumer->on_empty_range(
                    convert_right_bound(right_incl));
            }
        }

        continue_bool_t handle_pair(scoped_key_value_t &&) {
            unreachable();
        }

        btree_backfill_pre_atom_consumer_t *pre_atom_consumer;
        key_range_t range;
        repli_timestamp_t reference_timestamp;
        value_sizer_t *sizer;
    } callback;
    callback.pre_atom_consumer = pre_atom_consumer;
    callback.range = range;
    callback.reference_timestamp = reference_timestamp;
    callback.sizer = sizer;
    if (continue_bool_t::ABORT == btree_depth_first_traversal(
            superblock, range, &callback, access_t::read, FORWARD, release_superblock,
            interruptor)) {
        return continue_bool_t::ABORT;
    }
    return pre_atom_consumer->on_empty_range(range.right);
}

/* The `backfill_atom_loader_t` gets backfill atoms from the `backfill_atom_preparer_t`,
but that the actual row values have not been loaded into the atoms yet. It loads the
values from the cache and then passes the atoms on to the `backfill_atom_consumer_t`. */
class backfill_atom_loader_t {
public:
    backfill_atom_loader_t(
            btree_backfill_atom_consumer_t *_atom_consumer, cond_t *_abort_cond) :
        atom_consumer(_atom_consumer), abort_cond(_abort_cond), semaphore(16) { }

    /* `on_atom()` and `on_empty_range()` will be called in lexicographical order. They
    will always be called from the same coroutine, so if a call blocks the traversal will
    be paused until it returns. */

    /* The atom passed to `on_atom` is complete except for the `value` field of each
    pair. If the pair has a value, then instead of containing that value, `pair->value`
    will contain a pointer into `buf_read.get_data_read()` which can be used to actually
    load the value. The pointer is stored in the `std::vector<char>` that would normally
    be supposed to store the value. This is kind of a hack, but it will work and it's
    localized to these two types. */
    void on_atom(
            backfill_atom_t &&atom,
            const counted_t<counted_buf_lock_and_read_t> &buf) {
        new_semaphore_acq_t sem_acq(&semaphore, atom.pairs.size());
        cond_t non_interruptor;   /* RSI(raft): figure out interruption */
        wait_interruptible(sem_acq.acquisition_signal(), &non_interruptor);
        coro_t::spawn_sometime(std::bind(
            &backfill_atom_loader_t::handle_atom, this,
            std::move(atom), buf, std::move(sem_acq),
            fifo_source.enter_write(), drainer.lock()));
    }

    void on_empty_range(const btree_key_t *right_incl) {
        new_semaphore_acq_t sem_acq(&semaphore, 1);
        cond_t non_interruptor;   /* RSI(raft): figure out interruption */
        wait_interruptible(sem_acq.acquisition_signal(), &non_interruptor);
        coro_t::spawn_sometime(std::bind(
            &backfill_atom_loader_t::handle_empty_range, this,
            convert_right_bound(right_incl), std::move(sem_acq),
            fifo_source.enter_write(), drainer.lock()));
    }

    void finish(signal_t *interruptor) {
        fifo_enforcer_sink_t::exit_write_t exit_write(
            &fifo_sink, fifo_source.enter_write());
        wait_interruptible(&exit_write, interruptor);
    }

private:
    void handle_atom(
            backfill_atom_t &atom,
            const counted_t<counted_buf_lock_and_read_t> &buf,
            const new_semaphore_acq_t &,
            fifo_enforcer_write_token_t token,
            auto_drainer_t::lock_t keepalive) {
        try {
            pmap(atom.pairs.size(), [&](size_t i) {
                try {
                    if (!static_cast<bool>(atom.pairs[i].value)) {
                        /* It's a deletion; we don't need to load anything. */
                        return;
                    }
                    rassert(atom.pairs[i].value->size() == sizeof(void *));
                    void *value_ptr = *reinterpret_cast<void *const *>(
                        atom.pairs[i].value->data());
                    atom.pairs[i].value->clear();
                    atom_consumer->copy_value(buf_parent_t(&buf->lock), value_ptr,
                        keepalive.get_drain_signal(), &*atom.pairs[i].value);
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
            if (continue_bool_t::ABORT == atom_consumer->on_atom(std::move(atom))) {
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
            if (continue_bool_t::ABORT == atom_consumer->on_empty_range(threshold)) {
                abort_cond->pulse();
            }
        } catch (const interrupted_exc_t &) {
            /* ignore */
        }
    }

    btree_backfill_atom_consumer_t *atom_consumer;
    cond_t *abort_cond;
    new_semaphore_t semaphore;
    fifo_enforcer_source_t fifo_source;
    fifo_enforcer_sink_t fifo_sink;
    auto_drainer_t drainer;
};

/* `backfill_atom_preparer_t` visits leaf nodes using callbacks from the
`btree_depth_first_traversal()`. At each leaf node, it constructs a series of
`backfill_atom_t`s describing the leaf, but doesn't set their values yet; in place of the
values, it stores a pointer to where the value can be loaded from the leaf. Then it
passes them to the `backfill_atom_loader_t` to do the actual loading. */
class backfill_atom_preparer_t : public depth_first_traversal_callback_t {
public:
    backfill_atom_preparer_t(
            value_sizer_t *_sizer,
            repli_timestamp_t _reference_timestamp,
            btree_backfill_pre_atom_producer_t *_pre_atom_producer,
            signal_t *_abort_cond,
            backfill_atom_loader_t *_loader) :
        sizer(_sizer), reference_timestamp(_reference_timestamp),
        pre_atom_producer(_pre_atom_producer), abort_cond(_abort_cond), loader(_loader)
        { }

private:
    /* If `abort_cond` is pulsed we want to abort the traversal. The other methods use
    `return get_continue()` as a way to say "continue the traversal unless `abort_cond`
    is pulsed". */
    continue_bool_t get_continue() {
        return abort_cond->is_pulsed()
            ? continue_bool_t::ABORT : continue_bool_t::CONTINUE;
    }

    continue_bool_t filter_range_ts(
            const btree_key_t *left_excl_or_null,
            const btree_key_t *right_incl,
            repli_timestamp_t timestamp,
            bool *skip_out) {
        bool has_pre_atoms;
        if (continue_bool_t::ABORT == pre_atom_producer->peek_range(
                left_excl_or_null, right_incl, &has_pre_atoms)) {
            return continue_bool_t::ABORT;
        }
        *skip_out = timestamp <= reference_timestamp && !has_pre_atoms;
        if (*skip_out) {
            loader->on_empty_range(right_incl);
            /* There are no pre atoms in the range, but we need to call `consume()`
            anyway so that our calls to the `pre_atom_producer` are consecutive. */
            continue_bool_t cont = pre_atom_producer->consume_range(
                left_excl_or_null, right_incl,
                [](const backfill_pre_atom_t &) { unreachable(); });
            if (cont == continue_bool_t::ABORT) {
                return continue_bool_t::ABORT;
            }
        }
        return get_continue();
    }

    continue_bool_t handle_pre_leaf(
            const counted_t<counted_buf_lock_and_read_t> &buf,
            const btree_key_t *left_excl_or_null,
            const btree_key_t *right_incl,
            bool *skip_out) {
        *skip_out = true;
        key_range_t leaf_range(
            left_excl_or_null == nullptr
                ? key_range_t::bound_t::none : key_range_t::bound_t::open,
            left_excl_or_null,
            key_range_t::bound_t::closed,
            right_incl);
        const leaf_node_t *lnode = static_cast<const leaf_node_t *>(
            buf->read->get_data_read());

        repli_timestamp_t min_deletion_timestamp =
            leaf::min_deletion_timestamp(sizer, lnode, buf->lock.get_recency());
        if (min_deletion_timestamp > reference_timestamp) {
            /* We're not going to use these pre atoms, but we need to call `consume()`
            anyway so that our calls to the `pre_atom_producer` are consecutive. */
            continue_bool_t cont =
                pre_atom_producer->consume_range(left_excl_or_null, right_incl,
                    [](const backfill_pre_atom_t &) {} );
            if (cont == continue_bool_t::ABORT) {
                return continue_bool_t::ABORT;
            }

            /* We might be missing deletion entries, so re-transmit the entire node as a
            single `backfill_atom_t` */
            backfill_atom_t atom;
            atom.min_deletion_timestamp = min_deletion_timestamp;
            atom.range = leaf_range;

            /* Create a `backfill_atom_t::pair_t` for each key-value pair in the leaf
            node, but don't load the values yet */
            leaf::visit_entries(
                sizer, lnode, buf->lock.get_recency(),
                [&](const btree_key_t *key, repli_timestamp_t timestamp,
                        const void *value_or_null) -> continue_bool_t {
                    /* The leaf node might be partially outside the range of the
                    backfill, so we might have to skip some keys */
                    if (!leaf_range.contains_key(key)) {
                        return continue_bool_t::CONTINUE;
                    }

                    size_t i = atom.pairs.size();
                    atom.pairs.resize(i + 1);
                    atom.pairs[i].key.assign(key);
                    atom.pairs[i].recency = timestamp;
                    if (value_or_null != nullptr) {
                        /* Store `value_or_null` in the `value` field as a sequence of
                        8 (or 4 or whatever) `char`s describing its actual pointer value.
                        */
                        atom.pairs[i].value = std::vector<char>(
                            reinterpret_cast<const char *>(&value_or_null),
                            reinterpret_cast<const char *>(1 + &value_or_null));
                    }
                    return continue_bool_t::CONTINUE;
                });

            /* `visit_entries()` returns entries in timestamp order; we need to sort
            `atom.pairs` now to put it in lexicographical order */
            std::sort(atom.pairs.begin(), atom.pairs.end(),
                [](const backfill_atom_t::pair_t &p1,
                        const backfill_atom_t::pair_t &p2) {
                    return p1.key < p2.key;
                });

            loader->on_atom(std::move(atom), buf);

            return get_continue();

        } else {
            /* For each pre atom, make a backfill atom (which is initially empty) */
            std::list<backfill_atom_t> atoms_from_pre;
            continue_bool_t cont =
                pre_atom_producer->consume_range(left_excl_or_null, right_incl,
                    [&](const backfill_pre_atom_t &pre_atom) {
                        rassert(atoms_from_pre.empty() ||
                            key_range_t::right_bound_t(pre_atom.range.left) >=
                                atoms_from_pre.back().range.right);
                        backfill_atom_t atom;
                        atom.range = pre_atom.range.intersection(leaf_range);
                        atom.min_deletion_timestamp = min_deletion_timestamp;
                        atoms_from_pre.push_back(atom);
                    });
            if (cont == continue_bool_t::ABORT) {
                return continue_bool_t::ABORT;
            }

            /* Find each key-value pair or deletion entry that falls within the range of
            a pre atom or that changed since `reference_timestamp`. If it falls within
            the range of a pre atom, put it into the corresponding atom in
            `atoms_from_pre`; otherwise, make a new atom for it in `atoms_from_time`. */
            std::list<backfill_atom_t> atoms_from_time;
            leaf::visit_entries(
                sizer, lnode, buf->lock.get_recency(),
                [&](const btree_key_t *key, repli_timestamp_t timestamp,
                        const void *value_or_null) -> continue_bool_t {
                    /* The leaf node might be partially outside the range of the
                    backfill, so we might have to skip some keys */
                    if (!leaf_range.contains_key(key)) {
                        return continue_bool_t::CONTINUE;
                    }

                    /* In the most common case, `atoms_from_pre` is totally empty. Since
                    we ignore entries that are at `reference_timestamp` or older unless
                    they are in a pre atom, we can optimize things slightly by aborting
                    the leaf node iteration early. */
                    if (timestamp <= reference_timestamp && atoms_from_pre.empty()) {
                        return continue_bool_t::ABORT;
                    }

                    /* We'll set `atom` to the `backfill_atom_t` where this key-value
                    pair should be inserted. First we check if there's an atom in
                    `atoms_from_pre` that contains the key. If not, we'll create a new
                    atom in `atoms_from_time`. */
                    backfill_atom_t *atom = nullptr;
                    /* Linear search sucks, but there probably won't be a whole lot of
                    pre atoms, so it's OK for now. */
                    for (backfill_atom_t &a : atoms_from_pre) {
                        if (a.range.contains_key(key)) {
                            atom = &a;
                            break;
                        }
                    }
                    if (atom == nullptr) {
                        /* We didn't find an atom in `atoms_from_pre`. */
                        if (timestamp > reference_timestamp) {
                            /* We should create a new atom for this key-value pair */
                            atoms_from_time.push_back(backfill_atom_t());
                            atom = &atoms_from_time.back();
                            atom->range = key_range_t(key);
                            atom->min_deletion_timestamp =
                                repli_timestamp_t::distant_past;
                        } else {
                            /* Ignore this key-value pair */
                            return continue_bool_t::CONTINUE;
                        }
                    }

                    rassert(atom->range.contains_key(key));
                    rassert(timestamp >= atom->min_deletion_timestamp);

                    size_t i = atom->pairs.size();
                    atom->pairs.resize(i + 1);
                    atom->pairs[i].key.assign(key);
                    atom->pairs[i].recency = timestamp;
                    if (value_or_null != nullptr) {
                        /* Store `value_or_null` in the `value` field as a sequence of
                        8 (or 4 or whatever) `char`s describing its actual pointer value.
                        */
                        atom->pairs[i].value = std::vector<char>(
                            reinterpret_cast<const char *>(&value_or_null),
                            reinterpret_cast<const char *>(1 + &value_or_null));
                    }
                    return continue_bool_t::CONTINUE;
                });

            /* `leaf::visit_entries` returns entries in timestamp order, but we want
            `atoms_from_time` to be in lexicographical order, so we need to sort it. Same
            with the `pairs` field of each atom in `atoms_from_pre`. */
            atoms_from_time.sort(
                [](const backfill_atom_t &a1, const backfill_atom_t &a2) {
                    return a1.range.left < a2.range.left;
                });
            for (backfill_atom_t &a : atoms_from_pre) {
                std::sort(a.pairs.begin(), a.pairs.end(),
                    [](const backfill_atom_t::pair_t &p1,
                            const backfill_atom_t::pair_t &p2) {
                        return p1.key < p2.key;
                    });
            }

            /* Merge `atoms_from_time` into `atoms_from_pre`, preserving order. */
            atoms_from_pre.merge(
                std::move(atoms_from_time),
                [](const backfill_atom_t &a1, const backfill_atom_t &a2) {
                    rassert(!a1.range.overlaps(a2.range));
                    return a1.range.left < a2.range.left;
                });

            /* Send the results to the loader */
            for (backfill_atom_t &a : atoms_from_pre) {
                loader->on_atom(std::move(a), buf);
            }
            loader->on_empty_range(right_incl);

            return get_continue();
        }
    }

    continue_bool_t handle_pair(scoped_key_value_t &&) {
        unreachable();
    }

    value_sizer_t *sizer;
    repli_timestamp_t reference_timestamp;
    btree_backfill_pre_atom_producer_t *pre_atom_producer;
    signal_t *abort_cond;
    backfill_atom_loader_t *loader;
};

continue_bool_t btree_send_backfill(
        superblock_t *superblock,
        release_superblock_t release_superblock,
        value_sizer_t *sizer,
        const key_range_t &range,
        repli_timestamp_t reference_timestamp,
        btree_backfill_pre_atom_producer_t *pre_atom_producer,
        btree_backfill_atom_consumer_t *atom_consumer,
        signal_t *interruptor) {
    cond_t abort_cond;
    backfill_atom_loader_t loader(atom_consumer, &abort_cond);
    backfill_atom_preparer_t preparer(
        sizer, reference_timestamp, pre_atom_producer, &abort_cond, &loader);
    if (continue_bool_t::ABORT == btree_depth_first_traversal(
            superblock, range, &preparer, access_t::read, FORWARD, release_superblock,
            interruptor)) {
        return continue_bool_t::ABORT;
    }
    loader.finish(interruptor);
    return atom_consumer->on_empty_range(range.right);
}

class backfill_deletion_timestamp_updater_t : public depth_first_traversal_callback_t {
public:
    backfill_deletion_timestamp_updater_t(value_sizer_t *s, repli_timestamp_t mdt) :
        sizer(s), min_deletion_timestamp(mdt) { }
    continue_bool_t handle_pre_leaf(
            const counted_t<counted_buf_lock_and_read_t> &buf,
            UNUSED const btree_key_t *left_excl_or_null,
            UNUSED const btree_key_t *right_incl,
            bool *skip_out) {
        *skip_out = true;
        buf_write_t buf_write(&buf->lock);
        leaf_node_t *lnode = static_cast<leaf_node_t *>(buf_write.get_data_write());
        leaf::erase_deletions(sizer, lnode, min_deletion_timestamp);
        return continue_bool_t::CONTINUE;
    }
    continue_bool_t handle_pair(scoped_key_value_t &&) {
        unreachable();
    }
private:
    value_sizer_t *sizer;
    repli_timestamp_t min_deletion_timestamp;
};

void btree_receive_backfill_atom_update_deletion_timestamps(
        superblock_t *superblock,
        release_superblock_t release_superblock,
        value_sizer_t *sizer,
        const backfill_atom_t &atom,
        signal_t *interruptor) {
    backfill_deletion_timestamp_updater_t updater(sizer, atom.min_deletion_timestamp);
    continue_bool_t res = btree_depth_first_traversal(
        superblock, atom.range, &updater, access_t::write, FORWARD, release_superblock,
        interruptor);
    guarantee(res == continue_bool_t::CONTINUE);
}

