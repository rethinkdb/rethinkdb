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

bool btree_backfill_pre_atoms(
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
        done_traversing_t handle_pair(scoped_key_value_t &&) {
            unreachable();
        }
        done_traversing_t handle_pre_leaf(
                const counted_t<counted_buf_lock_t> &buf_lock,
                const counted_t<counted_buf_read_t> &buf_read,
                const btree_key_t *left_excl_or_null,
                const btree_key_t *right_incl_or_null,
                bool *skip_out) {
            const leaf_node_t *lnode = static_cast<const leaf_node_t *>(
                buf_read->get_data_read());
            class cb_t : public leaf::entry_reception_callback_t {
            public:
                cb_t(btree_backfill_pre_atom_consumer_t *c, const key_range_t &ran,
                        const btree_key_t *l, const btree_key_t *r) :
                    pre_atom_consumer(c), range(ran),
                    left_excl_or_null(l), right_incl_or_null(r),
                    did_lose_deletions(false), did_stop(false) { }
                void lost_deletions() {
                    guarantee(!did_lose_deletions && !did_stop);
                    did_lose_deletions = true;
                    backfill_pre_atom_t pre_atom;
                    if (left_excl_or_null == nullptr) {
                        pre_atom.range.left = range.left;
                    } else {
                        pre_atom.range.left.assign(left_excl_or_null);
                        bool ok = pre_atom.range.left.increment();
                        guarantee(ok);
                    }
                    if (right_incl_or_null == nullptr) {
                        pre_atom.range.right = range.right;
                    } else {
                        pre_atom.range.right.key.assign(right_incl_or_null);
                        pre_atom.range.right.unbounded = true;
                        bool ok = pre_atom.range.right.increment();
                        guarantee(ok);
                    }
                    did_stop = !pre_atom_consumer->on_pre_atom(std::move(pre_atom));
                }
                void deletion(const btree_key_t *key, repli_timestamp_t) {
                    if (!did_lose_deletions && !did_stop && range.contains_key(key)) {
                        backfill_pre_atom_t pre_atom;
                        pre_atom.range.left.assign(key);
                        pre_atom.range.right.key.assign(key);
                        pre_atom.range.right.unbounded = false;
                        bool ok = pre_atom.range.right.increment();
                        guarantee(ok);
                        did_stop = !pre_atom_consumer->on_pre_atom(std::move(pre_atom));
                    }
                }
                void keys_values(
                        std::vector<const btree_key_t *> &&ks,
                        std::vector<const void *> &&,
                        std::vector<repli_timestamp_t> &&) {
                    if (did_lose_deletions) {
                        return;
                    }
                    for (const btree_key_t *key : ks) {
                        if (did_stop) {
                            return;
                        }
                        if (!range.contains_key(key)) {
                            continue;
                        }
                        backfill_pre_atom_t pre_atom;
                        pre_atom.range.left.assign(key);
                        pre_atom.range.right.key.assign(key);
                        pre_atom.range.right.unbounded = false;
                        bool ok = pre_atom.range.right.increment();
                        guarantee(ok);
                        did_stop = !pre_atom_consumer->on_pre_atom(std::move(pre_atom));
                    }
                }
                btree_backfill_pre_atom_consumer_t *pre_atom_consumer;
                const key_range_t &range;
                const btree_key_t *left_excl_or_null, *right_incl_or_null;
                bool did_lose_deletions, did_stop;
            } cb(pre_atom_consumer, range, left_excl_or_null, right_incl_or_null);
            leaf::dump_entries_since_time(sizer, lnode, since_when,
                buf_lock->get_recency(), &cb);
            *skip_out = true;
            return cb.did_stop ? done_traversing_t::YES : done_traversing_t::NO;
        }
        bool is_range_ts_interesting(
                UNUSED const btree_key_t *left_excl_or_null,
                UNUSED const btree_key_t *right_incl_or_null,
                repli_timestamp_t timestamp) {
            return timestamp > since_when;
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

#if 0
bool btree_backfill_atoms(
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

        done_traversing_t handle_pre_leaf(
                const counted_t<counted_buf_lock_t> &buf_lock,
                const counted_t<counted_buf_read_t> &buf_read,
                const btree_key_t *left_excl_or_null,
                const btree_key_t *right_incl_or_null,
                bool *skip_out) {
            const leaf_node_t *lnode = static_cast<const leaf_node_t *>(
                buf_read->get_data_read());
            class cb_t : public leaf::entry_reception_callback_t {
            public:
                cb_t(std::list<backfill_atom_t> *as, const key_range_t &ran,
                        const btree_key_t *l, const btree_key_t *r) :
                    atoms(as), range(ran), left_excl_or_null(l), right_incl_or_null(r),
                    did_lose_deletions(false) { }
                void lost_deletions() {
                    guarantee(did_lose_deletions);
                    did_lose_deletions = true;
                    atoms->push_back(backfill_atom_t());
                    backfill_atom_t *atom = &atoms.back();
                    if (left_excl_or_null == nullptr) {
                        atom->range.left = range.left;
                    } else {
                        atom->range.left.assign(left_excl_or_null);
                        bool ok = atom->range.left.increment();
                        guarantee(ok);
                    }
                    if (right_incl_or_null == nullptr) {
                        atom->range.right = range.right;
                    } else {
                        atom->range.right.key.assign(right_incl_or_null);
                        atom->range.right.unbounded = true;
                        bool ok = atom->range.right.increment();
                        guarantee(ok);
                    }
                }
                void deletion(const btree_key_t *key, repli_timestamp_t timestamp) {
                    backfill_atom_t *atom;
                    if (did_lose_deletions) {
                        atom = &atoms.back();
                        guarantee(atom->range.contains_key(key));
                    } else {
                        atoms.push_back(backfill_atom_t());
                        atom = &atoms.back();
                        atom.range.left.assign(key);
                        atom.range.right.key.assign(key);
                        atom.range.right.unbounded = false;
                        bool ok = atom.range.right.increment();
                        guarantee(ok);
                    }
                    backfill_atom_t::pair_t pair;
                    pair.key.assign(key);
                    pair.timestamp = timestamp;
                    atom->pairs.push_back(std::move(pair));
                }
                void keys_values(
                        std::vector<const btree_key_t *> &&ks,
                        std::vector<const void *> &&,
                        std::vector<repli_timestamp_t> &&) {
                    if (did_lose_deletions) {
                        return;
                    }
                    std::sort(ks.begin(), ks.end(),
                        [&](const btree_key_t *k1, const btree_key_t *k2) {
                            return btree_key_cmp(k1, k2) == -1;
                        });
                    for (const btree_key_t *key : ks) {
                        atoms.push_back(backfill_atom_t());
                        atom = &atoms.back();
                        atom.range.left.assign(key);
                        atom.range.right.key.assign(key);
                        atom.range.right.unbounded = false;
                        bool ok = atom.range.right.increment();
                        guarantee(ok);
                    }
                }
                std::list<backfill_atom_t> atoms;
                const key_range_t &range;
                const btree_key_t *left_excl_or_null, *right_incl_or_null;
                bool did_lose_deletions;
            } cb(range, left_excl_or_null, right_incl_or_null);
            leaf::dump_entries_since_time(sizer, lnode, since_when,
                buf_lock->get_recency(), &cb);
            *skip_out = false;
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

        done_traversing_t handle_pair(
                scoped_key_value_t &&keyvalue,
                concurrent_traversal_fifo_enforcer_signal_t waiter) {
            /* Prepare a `backfill_atom_t::pair_t` that describes this key-value pair.
            This may block, but multiple copies will run concurrently. */
            backfill_atom_t::pair_t pair;
            pair.key.assign(keyvalue.key());
            pair.timestamp = keyvalue.timestamp();
            pair.value = boost::make_optional(std::vector<char>());
            atom_consumer->copy_value(keyvalue.expose_buf(), keyvalue.value(),
                interruptor, &*pair.value);

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
                bool cont = atom_consumer->on_atom(std::move(atoms_copying.front()));
                atoms_copying.pop_front();
                if (!cont) {
                    return done_traversing_t::YES;
                }
            }

            /* OK, now get a pointer to our atom. */
            backfill_atom_t *atom;
            if (!atoms_copying.empty()) {
                atom = &atoms_copying.front();
            } else {
                atom = &atoms_filtering.front();
            }
            guarantee(atom->range.contains_key(keyvalue.key()));

            atom->push_back(std::move(pair));
            return done_traversing_t::NO;
        }

        std::list<backfill_atom_t> atoms_copying, atoms_filtering;
    };
}
#endif


