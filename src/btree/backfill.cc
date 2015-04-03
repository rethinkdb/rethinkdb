// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/backfill.hpp"

#include "btree/depth_first_traversal.hpp"
#include "btree/leaf_node.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(backfill_pre_atom_t, range);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(backfill_atom_t::pair_t, key, recency, value);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(backfill_atom_t, range, pairs);

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

continue_bool_t btree_backfill_pre_atoms(
        superblock_t *superblock,
        release_superblock_t release_superblock,
        value_sizer_t *sizer,
        const key_range_t &range,
        repli_timestamp_t since_when,
        btree_backfill_pre_atom_consumer_t *pre_atom_consumer,
        /* RSI(raft): Respect interruptor */
        UNUSED signal_t *interruptor) {
    class callback_t : public depth_first_traversal_callback_t {
    public:
        bool is_range_ts_interesting(
                UNUSED const btree_key_t *left_excl_or_null,
                UNUSED const btree_key_t *right_incl,
                repli_timestamp_t timestamp) {
            return timestamp > since_when;
        }

        continue_bool_t handle_pre_leaf(
                const counted_t<counted_buf_lock_t> &buf_lock,
                const counted_t<counted_buf_read_t> &buf_read,
                const btree_key_t *left_excl_or_null,
                const btree_key_t *right_incl,
                bool *skip_out) {
            *skip_out = true;
            const leaf_node_t *lnode = static_cast<const leaf_node_t *>(
                buf_read->get_data_read());
            repli_timestamp_t cutoff =
                leaf::deletion_cutoff_timestamp(sizer, lnode, buf_lock->get_recency());
            if (cutoff > since_when) {
                /* We might be missing deletion entries, so re-transmit the entire node
                */
                backfill_pre_atom_t pre_atom;
                pre_atom.range = key_range_t(
                    left_excl_or_null == nullptr
                        ? key_range_t::bound_t::none : key_range_t::bound_t::open,
                    left_excl_or_null,
                    key_range_t::bound_t::closed,
                    right_incl);
                if (continue_bool_t::ABORT ==
                        pre_atom_consumer->on_pre_atom(std::move(pre_atom))) {
                    return continue_bool_t::ABORT;
                }
            } else {
                std::vector<const btree_key_t *> keys;
                leaf::visit_entries(
                    sizer, lnode, buf_lock->get_recency(),
                    [&](const btree_key_t *key, repli_timestamp_t timestamp,
                            const void *) -> bool {
                        if ((left_excl_or_null != nullptr &&
                                    btree_key_cmp(key, left_excl_or_null) != 1)
                                || btree_key_cmp(key, right_incl) == 1) {
                            return true;
                        }
                        if (timestamp <= since_when) {
                            return false;
                        }
                        keys.push_back(key);
                        return true;
                    });
                std::sort(keys.begin(), keys.end(),
                    [](const btree_key_t *k1, const btree_key_t *k2) {
                        return btree_key_cmp(k1, k2) == -1;
                    });
                for (const btree_key_t *key : keys) {
                    backfill_pre_atom_t pre_atom;
                    pre_atom.range = key_range_t(key);
                    if (continue_bool_t::ABORT ==
                            pre_atom_consumer->on_pre_atom(std::move(pre_atom))) {
                        return continue_bool_t::ABORT;
                    }
                }
            }
            return continue_bool_t::CONTINUE;
        }

        continue_bool_t handle_pair(scoped_key_value_t &&) {
            unreachable();
        }

        btree_backfill_pre_atom_consumer_t *pre_atom_consumer;
        key_range_t range;
        repli_timestamp_t since_when;
        value_sizer_t *sizer;
    } callback;
    callback.pre_atom_consumer = pre_atom_consumer;
    callback.range = range;
    callback.since_when = since_when;
    callback.sizer = sizer;
    return btree_depth_first_traversal(
        superblock,
        range,
        &callback,
        FORWARD,
        release_superblock);
}

continue_bool_t btree_backfill_atoms(
        superblock_t *superblock,
        release_superblock_t release_superblock,
        value_sizer_t *sizer,
        const key_range_t &range,
        repli_timestamp_t since_when,
        btree_backfill_pre_atom_producer_t *pre_atom_producer,
        btree_backfill_atom_consumer_t *atom_consumer,
        signal_t *interruptor) {
    class callback_t : public concurrent_traversal_callback_t {
    public:
        bool is_range_ts_interesting(
                const btree_key_t *left_excl_or_null,
                const btree_key_t *right_incl,
                repli_timestamp_t timestamp) {
            return timestamp > since_when
                || pre_atom_producer->peek_range(left_excl_or_null, right_incl);
        }

        continue_bool_t handle_pre_leaf(
                const counted_t<counted_buf_lock_t> &buf_lock,
                const counted_t<counted_buf_read_t> &buf_read,
                const btree_key_t *left_excl_or_null,
                const btree_key_t *right_incl,
                bool *skip_out) {
            *skip_out = false;
            key_range_t leaf_range(
                left_excl_or_null == nullptr
                    ? key_range_t::bound_t::none : key_range_t::bound_t::open,
                left_excl_or_null,
                key_range_t::bound_t::closed,
                right_incl);
            const leaf_node_t *lnode = static_cast<const leaf_node_t *>(
                buf_read->get_data_read());
            repli_timestamp_t cutoff =
                leaf::deletion_cutoff_timestamp(sizer, lnode, buf_lock->get_recency());
            if (cutoff > since_when) {
                /* We might be missing deletion entries, so re-transmit the entire node
                as a single `backfill_atom_t` */
                backfill_atom_t atom;
                atom.deletion_cutoff_timestamp = cutoff;
                atom.range = leaf_range;
                atoms_filtering.push_back(atom);
                return continue_bool_t::CONTINUE;
            } else {
                /* For each pre atom, make a backfill atom (which is initially empty) */
                std::list<backfill_atom_t> atoms_from_pre;
                pre_atom_producer->consume_range(left_excl_or_null, right_incl,
                    [&](const backfill_pre_atom_t *pre_atom) {
                        backfill_atom_t atom;
                        atom.range = pre->range.intersection(leaf_range);
                        atom.deletion_cutoff_timestamp = cutoff;
                        atoms_from_pre.push_back(atom);
                    });

                /* Find each key-value pair or deletion entry that falls within the range
                of a pre atom or that changed since `since_when`. If it falls within the
                range of a pre atom, put it into the corresponding atom in
                `atoms_from_pre`; otherwise, make a new atom for it in `atoms_from_time`.
                */
                std::list<backfill_atom_t> atoms_from_time;
                leaf::visit_entries(
                    sizer, lnode, buf_lock->get_recency(),
                    [&](const btree_key_t *key, repli_timestamp_t timestamp,
                            const void *value_or_null) -> bool {
                        /* The leaf node might be partially outside the range of the
                        backfill, so we might have to skip some keys */
                        if (!leaf_range.contains_key(key)) {
                            return true;
                        }

                        /* In the most common case, `atoms_from_pre` is totally empty.
                        Since we ignore entries that are older than `since_when` unless
                        they are in a pre atom, we can optimize things slightly by
                        aborting iteration early. */
                        if (timestamp <= since_when && atoms_from_pre.empty()) {
                            return false;
                        }

                        /* We'll set `atom` to the `backfill_atom_t` where this key-value
                        pair should be inserted. First we check if there's an atom in
                        `atoms_from_pre` that contains the key. If not, we'll create a
                        new atom in `atoms_from_time`. */
                        backfill_atom_t *atom = nullptr;
                        for (backfill_atom_t &a : atoms_from_pre) {
                            if (a.range.contains_key(key)) {
                                atom = &a;
                                break;
                            }
                        }
                        if (atom != nullptr) {
                            /* We didn't find an atom in `atoms_from_pre`. */
                            if (timestamp > since_when) {
                                /* We should create a new atom for this key-value pair */
                                atoms_from_time.push_back(backfill_atom_t());
                                atom = &atoms_from_time.back();
                                atom->range = key_range_t(key);
                                atom->deletion_cutoff_timestamp =
                                    repli_timestamp_t::distant_past;
                            } else {
                                /* Ignore this key-value pair */
                                return true;
                            }
                        }

                        rassert(atom->range.contains_key(key));
                        rassert(timestamp >= atom->deletion_cutoff_timestamp);

                        size_t i = atom->pairs.size();
                        atom->pairs.resize(i + 1);
                        atom->pairs[i].key.assign(key);
                        atom->pairs[i].recency = timestamp;
                        if (value_or_null != nullptr) {
                            /* Make a placeholder to indicate that we expect a value; it
                            will be filled in by `handle_pair()` */
                            atom->pairs[i].value =
                                boost::make_optional(std::vector<char>());
                        }
                        return true;
                    });

                /* `leaf::visit_entries` doesn't necessarily go in lexicographical order.
                So `atoms_from_time` and the pair-vectors inside the atoms in
                `atoms_from_pre` are currently unsorted. So now we sort them. */
                atoms_from_time.sort(
                    [](const backfill_atom_t &a1, const backfill_atom_t &a2) {
                        return a1.range.left < a2.range.left;
                    });
                for (auto &&atom : atoms_from_pre) {
                    std::sort(atom.pairs.begin(), atom.pairs.end(),
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

                /* Put the resulting atoms into `atoms_filtering` */
                atoms_filtering.splice(atoms_filtering.end(), std::move(atoms_from_pre));

                return continue_bool_t::CONTINUE;
            }
        }

        bool is_key_interesting(const btree_key_t *key) {
            /* Transfer atoms from `atoms_filtering` to `atoms_copying` until we find one
            that contains this key or comes after this key. */
            while (!atoms_filtering.empty()) {
                const key_range_t *r = &atoms_filtering.front().range;
                if (!r->right.unbounded &&
                        btree_key_cmp(r->right.key.btree_key(), key) <= 0) {
                    atoms_copying.splice(
                        atoms_copying.end(), atoms_filtering, atoms_filtering.begin());
                } else {
                    break;
                }
            }

            /* Return true if we found an atom that contains this key. */
            return !atoms_filtering.empty()
                && atoms_filtering.front().range.contains_key(key);
        }

        continue_bool_t handle_pair(
                scoped_key_value_t &&keyvalue,
                concurrent_traversal_fifo_enforcer_signal_t waiter) {
            /* Transfer the value of the key-value pair (the real contents in the blob,
            not the contents in the leaf node) into `buffer`. This may block, but
            multiple copies run concurrently. */
            std::vector<char> buffer;
            atom_consumer->copy_value(keyvalue.expose_buf(), keyvalue.value(),
                interruptor, &buffer);

            /* Wait for exclusive access */
            waiter.wait_interruptible();

            /* We already know that there's a `backfill_atom_t` that contains our key, or
            else `is_key_interesting()` would have returned `false` and `handle_pair()`
            would never have been called. Our `backfill_atom_t` is either somewhere in
            `atoms_copying` or at the very front of `atoms_filtering`. So we pop atoms
            off of `atoms_copying` until we see our key or reach the end. */
            while (!atoms_copying.empty() &&
                    !atoms_copying.front().range.contains_key(keyvalue.key())) {
                /* As we pop atoms off `atoms_copying`, we pass them to the callback. */
                if (continue_bool_t::ABORT ==
                        atom_consumer->on_atom(std::move(atoms_copying.front()))) {
                    return continue_bool_t::ABORT;
                }
                atoms_copying.pop_front();
            }

            /* OK, now get a pointer to our atom. */
            backfill_atom_t *atom;
            if (!atoms_copying.empty()) {
                atom = &atoms_copying.front();
            } else {
                atom = &atoms_filtering.front();
            }
            rassert(atom->range.contains_key(keyvalue.key()));

            /* Find the `backfill_atom_t::pair_t` for our key. We know that the pair must
            exist because `handle_pre_leaf()` should have created it. */
            auto it = std::lower_bound(
                atom->pairs.begin(), atoms->pairs.end(), keyvalue.key(),
                [](const backfill_atom_t::pair_t &p, const btree_key_t *k) {
                    return btree_key_cmp(p.key.btree_key(), k) == -1;
                });
            guarantee(it != atoms->pairs.end()
                && btree_key_cmp(it->key, keyvalue.key()); == 0)
            guarantee(static_cast<bool>(it->value), "handle_pair() called on something "
                "that handle_pre_leaf() thought was a deletion");

            /* Move the value from `buffer` into the pair */
            *it->value = std::move(buffer);

            return continue_bool_t::CONTINUE;
        }

        value_sizer_t *sizer;
        repli_timestamp_t since_when;
        backfill_pre_atom_producer_t *pre_atom_producer;
        backfill_atom_consumer_t *atom_consumer;
        std::list<backfill_atom_t> atoms_copying, atoms_filtering;
    } callback;
    callback.sizer = sizer;
    callback.since_when = since_when;
    callback.pre_atom_producer = pre_atom_producer;
    callback.atom_consumer = atom_consumer;

    /* Perform the traversal. This will call `atom_consumer->on_atom()` as it goes. */
    if (continue_bool_t::ABORT == btree_concurrent_traversal(
            superblock,
            range,
            &callback,
            FORWARD,
            release_superblock)) {
        return continue_bool_t::ABORT;
    }

    guarantee(callback.atoms_copying.empty());
    guarantee(callback.atoms_filtering.empty());

    return continue_bool_t::CONTINUE;
}

