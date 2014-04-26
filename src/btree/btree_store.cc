// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/btree_store.hpp"

#include <functional>

#include "arch/runtime/coroutines.hpp"
#include "btree/operations.hpp"
#include "btree/secondary_operations.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/cache_balancer.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/disk_backed_queue.hpp"
#include "serializer/config.hpp"
#include "stl_utils.hpp"

sindex_not_ready_exc_t::sindex_not_ready_exc_t(
        std::string sindex_name)
    : info(strprintf("Index `%s` was accessed before its construction was finished "
                     " or while it was being deleted.",
                sindex_name.c_str()))
{ }

const char* sindex_not_ready_exc_t::what() const throw() {
    return info.c_str();
}

sindex_not_ready_exc_t::~sindex_not_ready_exc_t() throw() { }

template <class protocol_t>
btree_store_t<protocol_t>::btree_store_t(serializer_t *serializer,
                                         cache_balancer_t *balancer,
                                         const std::string &perfmon_name,
                                         bool create,
                                         perfmon_collection_t *parent_perfmon_collection,
                                         typename protocol_t::context_t *,
                                         io_backender_t *io_backender,
                                         const base_path_t &base_path)
    : store_view_t<protocol_t>(protocol_t::region_t::universe()),
      perfmon_collection(),
      io_backender_(io_backender), base_path_(base_path),
      perfmon_collection_membership(parent_perfmon_collection, &perfmon_collection, perfmon_name)
{
    cache.init(new cache_t(serializer, balancer, &perfmon_collection));
    general_cache_conn.init(new cache_conn_t(cache.get()));

    if (create) {
        vector_stream_t key;
        write_message_t msg;
        typename protocol_t::region_t kr = protocol_t::region_t::universe();
        msg << kr;
        key.reserve(msg.size());
        int res = send_write_message(&key, &msg);
        guarantee(!res);

        txn_t txn(general_cache_conn.get(), write_durability_t::HARD,
                  repli_timestamp_t::distant_past, 1);
        buf_lock_t superblock(&txn, SUPERBLOCK_ID, alt_create_t::create);
        btree_slice_t::init_superblock(&superblock, key.vector(), std::vector<char>());
    }

    btree.init(new btree_slice_t(cache.get(), &perfmon_collection, "primary"));

    // Initialize sindex slices
    {
        // Since this is the btree constructor, nothing else should be locking these
        // things yet, so this should work fairly quickly and does not need a real
        // interruptor.
        cond_t dummy_interruptor;
        read_token_pair_t token_pair;
        store_view_t<protocol_t>::new_read_token_pair(&token_pair);

        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        acquire_superblock_for_read(&token_pair.main_read_token, &txn,
                                    &superblock, &dummy_interruptor, false);

        buf_lock_t sindex_block
            = acquire_sindex_block_for_read(superblock->expose_buf(),
                                            superblock->get_sindex_block_id());

        std::map<std::string, secondary_index_t> sindexes;
        get_secondary_indexes(&sindex_block, &sindexes);

        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            secondary_index_slices.insert(it->second.id,
                                          new btree_slice_t(cache.get(),
                                                            &perfmon_collection,
                                                            it->first));
        }
    }
}

template <class protocol_t>
btree_store_t<protocol_t>::~btree_store_t() {
    assert_thread();
}

template <class protocol_t>
void btree_store_t<protocol_t>::read(
        DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
        const typename protocol_t::read_t &read,
        typename protocol_t::read_response_t *response,
        UNUSED order_token_t order_token,  // TODO
        read_token_pair_t *token_pair,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;

    acquire_superblock_for_read(&token_pair->main_read_token, &txn, &superblock,
                                interruptor,
                                read.use_snapshot());

    DEBUG_ONLY(check_metainfo(DEBUG_ONLY(metainfo_checker, ) superblock.get());)

    protocol_read(read, response, btree.get(), superblock.get(), interruptor);
}

template <class protocol_t>
void btree_store_t<protocol_t>::write(
        DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
        const metainfo_t& new_metainfo,
        const typename protocol_t::write_t &write,
        typename protocol_t::write_response_t *response,
        const write_durability_t durability,
        transition_timestamp_t timestamp,
        UNUSED order_token_t order_token,  // TODO
        write_token_pair_t *token_pair,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> real_superblock;
    const int expected_change_count = 2; // FIXME: this is incorrect, but will do for now
    acquire_superblock_for_write(timestamp.to_repli_timestamp(),
                                 expected_change_count, durability, token_pair,
                                 &txn, &real_superblock, interruptor);

    check_and_update_metainfo(DEBUG_ONLY(metainfo_checker, ) new_metainfo,
                              real_superblock.get());
    scoped_ptr_t<superblock_t> superblock(real_superblock.release());
    protocol_write(write, response, timestamp, btree.get(), &superblock,
                   interruptor);
}

// TODO: Figure out wtf does the backfill filtering, figure out wtf constricts delete range operations to hit only a certain hash-interval, figure out what filters keys.
template <class protocol_t>
bool btree_store_t<protocol_t>::send_backfill(
        const region_map_t<protocol_t, state_timestamp_t> &start_point,
        send_backfill_callback_t<protocol_t> *send_backfill_cb,
        typename protocol_t::backfill_progress_t *progress,
        read_token_pair_t *token_pair,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_backfill(&token_pair->main_read_token, &txn, &superblock, interruptor);

    buf_lock_t sindex_block
        = acquire_sindex_block_for_read(superblock->expose_buf(),
                                        superblock->get_sindex_block_id());

    region_map_t<protocol_t, binary_blob_t> unmasked_metainfo;
    get_metainfo_internal(superblock->get(), &unmasked_metainfo);
    region_map_t<protocol_t, binary_blob_t> metainfo = unmasked_metainfo.mask(start_point.get_domain());
    if (send_backfill_cb->should_backfill(metainfo)) {
        protocol_send_backfill(start_point, send_backfill_cb, superblock.get(), &sindex_block, btree.get(), progress, interruptor);
        return true;
    }
    return false;
}

template <class protocol_t>
void btree_store_t<protocol_t>::receive_backfill(
        const typename protocol_t::backfill_chunk_t &chunk,
        write_token_pair_t *token_pair,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> real_superblock;
    const int expected_change_count = 1; // TODO: this is not correct

    // We use HARD durability because we want backfilling to be throttled if we
    // receive data faster than it can be written to disk. Otherwise we might
    // exhaust the cache's dirty page limit and bring down the whole table.
    // Other than that, the hard durability guarantee is not actually
    // needed here.
    acquire_superblock_for_write(chunk.get_btree_repli_timestamp(),
                                 expected_change_count,
                                 write_durability_t::HARD,
                                 token_pair,
                                 &txn,
                                 &real_superblock,
                                 interruptor);

    scoped_ptr_t<superblock_t> superblock(real_superblock.release());
    protocol_receive_backfill(btree.get(),
                              std::move(superblock),
                              interruptor,
                              chunk);
}

template <class protocol_t>
void btree_store_t<protocol_t>::reset_data(
        const typename protocol_t::region_t &subregion,
        const metainfo_t &new_metainfo,
        write_token_pair_t *token_pair,
        const write_durability_t durability,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;

    // We're passing 2 for the expected_change_count based on the
    // reasoning that we're probably going to touch a leaf-node-sized
    // range of keys and that it won't be aligned right on a leaf node
    // boundary.
    // TOnDO that's not reasonable; reset_data() is sometimes used to wipe out
    // entire databases.
    const int expected_change_count = 2;
    acquire_superblock_for_write(repli_timestamp_t::invalid,
                                 expected_change_count,
                                 durability,
                                 token_pair,
                                 &txn,
                                 &superblock,
                                 interruptor);

    region_map_t<protocol_t, binary_blob_t> old_metainfo;
    get_metainfo_internal(superblock->get(), &old_metainfo);
    update_metainfo(old_metainfo, new_metainfo, superblock.get());

    protocol_reset_data(subregion,
                        btree.get(),
                        superblock.get(),
                        interruptor);
}

template <class protocol_t>
scoped_ptr_t<new_mutex_in_line_t> btree_store_t<protocol_t>::get_in_line_for_sindex_queue(
        buf_lock_t *sindex_block) {
    assert_thread();
    // The line for the sindex queue is there to guarantee that we push things to
    // the sindex queue(s) in the same order in which we held the sindex block.
    // It allows us to release the sindex block before starting to push data to the
    // sindex queues without sacrificing that guarantee.
    // Pushing data to those queues can take a while, and we wouldn't want to block
    // other transactions while we are doing that.

    guarantee(!sindex_block->empty());
    if (sindex_block->access() == access_t::write) {
        sindex_block->write_acq_signal()->wait();
    } else {
        // `bring_sindexes_up_to_date()` calls `lock_sindex_queue()` with only a read
        // acquisition. It is ok in that context, and we have to handle it here.
        sindex_block->read_acq_signal()->wait();
    }
    return scoped_ptr_t<new_mutex_in_line_t>(
            new new_mutex_in_line_t(&sindex_queue_mutex));
}

template <class protocol_t>
void btree_store_t<protocol_t>::register_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue,
            const new_mutex_in_line_t *acq) {
    assert_thread();
    acq->acq_signal()->wait_lazily_unordered();

    for (std::vector<internal_disk_backed_queue_t *>::iterator it = sindex_queues.begin();
            it != sindex_queues.end(); ++it) {
        guarantee(*it != disk_backed_queue);
    }
    sindex_queues.push_back(disk_backed_queue);
}

template <class protocol_t>
void btree_store_t<protocol_t>::deregister_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue,
            const new_mutex_in_line_t *acq) {
    assert_thread();
    acq->acq_signal()->wait_lazily_unordered();

    for (std::vector<internal_disk_backed_queue_t *>::iterator it = sindex_queues.begin();
            it != sindex_queues.end(); ++it) {
        if (*it == disk_backed_queue) {
            sindex_queues.erase(it);
            return;
        }
    }
}

template <class protocol_t>
void btree_store_t<protocol_t>::emergency_deregister_sindex_queue(
    internal_disk_backed_queue_t *disk_backed_queue) {
    assert_thread();
    drainer.assert_draining();
    new_mutex_in_line_t acq(&sindex_queue_mutex);
    acq.acq_signal()->wait_lazily_unordered();

    deregister_sindex_queue(disk_backed_queue, &acq);
}

template <class protocol_t>
void btree_store_t<protocol_t>::sindex_queue_push(const write_message_t &value,
                                                  const new_mutex_in_line_t *acq) {
    assert_thread();
    acq->acq_signal()->wait_lazily_unordered();

    for (auto it = sindex_queues.begin(); it != sindex_queues.end(); ++it) {
        (*it)->push(value);
    }
}

template <class protocol_t>
void btree_store_t<protocol_t>::sindex_queue_push(
        const scoped_array_t<write_message_t> &values,
        const new_mutex_in_line_t *acq) {
    assert_thread();
    acq->acq_signal()->wait_lazily_unordered();

    for (auto it = sindex_queues.begin(); it != sindex_queues.end(); ++it) {
        (*it)->push(values);
    }
}

template <class protocol_t>
void btree_store_t<protocol_t>::add_progress_tracker(
        map_insertion_sentry_t<uuid_u, const parallel_traversal_progress_t *> *sentry,
        uuid_u id, const parallel_traversal_progress_t *p) {
    assert_thread();
    sentry->reset(&progress_trackers, id, p);
}

template <class protocol_t>
progress_completion_fraction_t btree_store_t<protocol_t>::get_progress(uuid_u id) {
    if (!std_contains(progress_trackers, id)) {
        return progress_completion_fraction_t();
    } else {
        return progress_trackers[id]->guess_completion();
    }
}

// KSI: If we're going to have these functions at all, we could just pass the
// real_superblock_t directly.
template <class protocol_t>
buf_lock_t btree_store_t<protocol_t>::acquire_sindex_block_for_read(
        buf_parent_t parent,
        block_id_t sindex_block_id) {
    return buf_lock_t(parent, sindex_block_id, access_t::read);
}

template <class protocol_t>
buf_lock_t btree_store_t<protocol_t>::acquire_sindex_block_for_write(
        buf_parent_t parent,
        block_id_t sindex_block_id) {
    return buf_lock_t(parent, sindex_block_id, access_t::write);
}

template <class region_map_t>
bool has_homogenous_value(const region_map_t &metainfo, typename region_map_t::mapped_type state) {
    for (typename region_map_t::const_iterator it  = metainfo.begin();
                                               it != metainfo.end();
                                               ++it) {
        if (it->second != state) {
            return false;
        }
    }
    return true;
}

template <class protocol_t>
bool btree_store_t<protocol_t>::add_sindex(
        const std::string &id,
        const secondary_index_t::opaque_definition_t &definition,
        buf_lock_t *sindex_block) {
    secondary_index_t sindex;
    if (::get_secondary_index(sindex_block, id, &sindex)) {
        return false; // sindex was already created
    } else {
        {
            buf_lock_t sindex_superblock(sindex_block, alt_create_t::create);
            sindex.superblock = sindex_superblock.block_id();
            sindex.opaque_definition = definition;

            /* Notice we're passing in empty strings for metainfo. The metainfo in
             * the sindexes isn't used for anything but this could perhaps be
             * something that would give a better error if someone did try to use
             * it... on the other hand this code isn't exactly idiot proof even
             * with that. */
            btree_slice_t::init_superblock(&sindex_superblock,
                                           std::vector<char>(), std::vector<char>());
        }

        secondary_index_slices.insert(
            sindex.id, new btree_slice_t(cache.get(), &perfmon_collection, id));

        sindex.post_construction_complete = false;

        ::set_secondary_index(sindex_block, id, sindex);
        return true;
    }
}

std::string compute_sindex_deletion_id(uuid_u sindex_uuid) {
    // Add a '\0' at the end to make absolutely sure it can't collide with
    // regular sindex names.
    return "_DEL_" + uuid_to_str(sindex_uuid) + "\0";
}

template <class protocol_t>
void btree_store_t<protocol_t>::clear_sindex(
        secondary_index_t sindex,
        value_sizer_t<void> *sizer,
        const value_deleter_t *deleter,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    {
        /* Start a transaction (1). */
        write_token_pair_t token_pair;
        store_view_t<protocol_t>::new_write_token_pair(&token_pair);
        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        acquire_superblock_for_write(
            repli_timestamp_t::distant_past,
            64 /* Not the right value for sure */,
            write_durability_t::SOFT,
            &token_pair,
            &txn,
            &superblock,
            interruptor);

        /* Get the sindex block (1). */
        buf_lock_t sindex_block = acquire_sindex_block_for_write(
                superblock->expose_buf(),
                superblock->get_sindex_block_id());
        superblock->release();

        /* Clear out the index data */
        buf_lock_t sindex_superblock_lock(buf_parent_t(&sindex_block),
                                          sindex.superblock, access_t::write);
        sindex_block.reset_buf_lock();
        real_superblock_t sindex_superblock(std::move(sindex_superblock_lock));
        erase_all(sizer, deleter, &sindex_superblock, interruptor);
    }

    {
        /* Start a transaction (2). */
        write_token_pair_t token_pair;
        store_view_t<protocol_t>::new_write_token_pair(&token_pair);
        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        acquire_superblock_for_write(
            repli_timestamp_t::distant_past,
            64 /* Not the right value for sure */,
            write_durability_t::SOFT,
            &token_pair,
            &txn,
            &superblock,
            interruptor);

        /* Get the sindex block (2). */
        buf_lock_t sindex_block = acquire_sindex_block_for_write(
                superblock->expose_buf(),
                superblock->get_sindex_block_id());
        superblock->release();

        /* Now it's safe to completely delete the index */
        buf_lock_t sindex_superblock_lock(buf_parent_t(&sindex_block),
                                          sindex.superblock, access_t::write);
        sindex_superblock_lock.mark_deleted();
        ::delete_secondary_index(&sindex_block, compute_sindex_deletion_id(sindex.id));
        size_t num_erased = secondary_index_slices.erase(sindex.id);
        guarantee(num_erased == 1);
    }
}

template <class protocol_t>
void btree_store_t<protocol_t>::set_sindexes(
        const std::map<std::string, secondary_index_t> &sindexes,
        buf_lock_t *sindex_block,
        std::shared_ptr<value_sizer_t<void> > sizer,
        std::shared_ptr<deletion_context_t> live_deletion_context,
        std::shared_ptr<deletion_context_t> post_construction_deletion_context,
        std::set<std::string> *created_sindexes_out)
    THROWS_ONLY(interrupted_exc_t) {

    std::map<std::string, secondary_index_t> existing_sindexes;
    ::get_secondary_indexes(sindex_block, &existing_sindexes);

    for (auto it = existing_sindexes.begin(); it != existing_sindexes.end(); ++it) {
        if (!std_contains(sindexes, it->first)) {
            /* Mark the secondary index for deletion to get it out of the way
            (note that a new sindex with the same name can be created
            from now on, which is what we want). */
            bool success = mark_secondary_index_deleted(sindex_block, it->first);
            guarantee(success);

            /* If the index had been completely constructed, we must detach
             * its values since snapshots might be accessing it.  If on the other
             * hand the index has not finished post construction, it would be
             * incorrect to do so. The reason being that some of the values that
             * the sindex points to might have been deleted in the meantime
             * (the deletion would be on the sindex queue, but might not have
             * found its way into the index tree yet). */
            // TODO (daniel): Now that we don't have multi-protocol support anymore,
            //   we could get rid of the shared_ptr's for the deletion context and just
            //   create our own ones inside of the delayes_sindex_clearer coro right?
            //   Same for sizer.
            std::shared_ptr<deletion_context_t> actual_deletion_context =
                    it->second.post_construction_complete
                    ? live_deletion_context
                    : post_construction_deletion_context;

            /* Delay actually clearing the secondary index, so we can
            release the sindex_block now, rather than having to wait for
            clear_sindex() to finish. */
            struct delayed_sindex_clearer {
                static void clear(btree_store_t<protocol_t> *btree_store,
                                  secondary_index_t sindex,
                                  std::shared_ptr<value_sizer_t<void> > csizer,
                                  std::shared_ptr<deletion_context_t> deletion_context,
                                  auto_drainer_t::lock_t store_keepalive) {
                    try {
                        /* Clear the sindex. */
                        btree_store->clear_sindex(
                            sindex,
                            csizer.get(),
                            deletion_context->in_tree_deleter(),
                            store_keepalive.get_drain_signal());
                    } catch (const interrupted_exc_t &e) {
                        /* Ignore. The sindex deletion will continue when the store
                        is next started up. */
                    }
                }
            };
            coro_t::spawn_sometime(std::bind(&delayed_sindex_clearer::clear,
                                             this,
                                             it->second,
                                             sizer,
                                             actual_deletion_context,
                                             drainer.lock()));
        }
    }

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (!std_contains(existing_sindexes, it->first)) {
            secondary_index_t sindex(it->second);
            {
                buf_lock_t sindex_superblock(sindex_block, alt_create_t::create);
                sindex.superblock = sindex_superblock.block_id();
                btree_slice_t::init_superblock(&sindex_superblock,
                                               std::vector<char>(),
                                               std::vector<char>());
            }

            secondary_index_slices.insert(it->second.id,
                                          new btree_slice_t(cache.get(),
                                                            &perfmon_collection,
                                                            it->first));

            sindex.post_construction_complete = false;

            ::set_secondary_index(sindex_block, it->first, sindex);

            created_sindexes_out->insert(it->first);
        }
    }
}

template <class protocol_t>
bool btree_store_t<protocol_t>::mark_index_up_to_date(const std::string &id,
                                                      buf_lock_t *sindex_block)
    THROWS_NOTHING {
    secondary_index_t sindex;
    bool found = ::get_secondary_index(sindex_block, id, &sindex);

    if (found) {
        sindex.post_construction_complete = true;

        ::set_secondary_index(sindex_block, id, sindex);
    }

    return found;
}

template <class protocol_t>
bool btree_store_t<protocol_t>::mark_index_up_to_date(uuid_u id,
                                                      buf_lock_t *sindex_block)
    THROWS_NOTHING {
    secondary_index_t sindex;
    bool found = ::get_secondary_index(sindex_block, id, &sindex);

    if (found) {
        sindex.post_construction_complete = true;

        ::set_secondary_index(sindex_block, id, sindex);
    }

    return found;
}

template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::drop_sindex(
        const std::string &id,
        buf_lock_t &&sindex_block,
        std::shared_ptr<value_sizer_t<void> > sizer,
        std::shared_ptr<deletion_context_t> live_deletion_context,
        std::shared_ptr<deletion_context_t> post_construction_deletion_context)
        THROWS_ONLY(interrupted_exc_t) {

    buf_lock_t local_sindex_block(std::move(sindex_block));
    /* Remove reference in the super block */
    secondary_index_t sindex;
    if (!::get_secondary_index(&local_sindex_block, id, &sindex)) {
        return false;
    } else {
        // Similar to `btree_store_t::set_sindexes()`, we have to pick a deletion
        // context based on whether the sindex had finished post construction or not.
        std::shared_ptr<deletion_context_t> actual_deletion_context =
                sindex.post_construction_complete
                ? live_deletion_context
                : post_construction_deletion_context;

        /* Mark the secondary index as deleted */
        bool success = mark_secondary_index_deleted(&local_sindex_block, id);
        guarantee(success);

        /* Clear the sindex later. It starts its own transaction and we don't
        want to deadlock because we're still holding locks. */
        struct delayed_sindex_clearer {
            static void clear(btree_store_t<protocol_t> *btree_store,
                              secondary_index_t csindex,
                              std::shared_ptr<value_sizer_t<void> > csizer,
                              std::shared_ptr<deletion_context_t> deletion_context,
                              auto_drainer_t::lock_t store_keepalive) {
                try {
                    /* Clear the sindex. */
                    btree_store->clear_sindex(
                        csindex,
                        csizer.get(),
                        deletion_context->in_tree_deleter(),
                        store_keepalive.get_drain_signal());
                } catch (const interrupted_exc_t &e) {
                    /* Ignore. The sindex deletion will continue when the store
                    is next started up. */
                }
            }
        };
        coro_t::spawn_sometime(std::bind(&delayed_sindex_clearer::clear,
                                         this,
                                         sindex,
                                         sizer,
                                         actual_deletion_context,
                                         drainer.lock()));
    }
    return true;
}

template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::mark_secondary_index_deleted(
        buf_lock_t *sindex_block,
        const std::string &id) {
    secondary_index_t sindex;
    bool success = get_secondary_index(sindex_block, id, &sindex);
    if (!success) {
        return false;
    }

    // Delete the current entry
    success = delete_secondary_index(sindex_block, id);
    guarantee(success);

    // Insert the new entry under a different name
    const std::string sindex_del_name = compute_sindex_deletion_id(sindex.id);
    sindex.being_deleted = true;
    set_secondary_index(sindex_block, sindex_del_name, sindex);
    return true;
}

template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::acquire_sindex_superblock_for_read(
        const std::string &id,
        superblock_t *superblock,
        scoped_ptr_t<real_superblock_t> *sindex_sb_out,
        std::vector<char> *opaque_definition_out,
        uuid_u *sindex_uuid_out)
    THROWS_ONLY(sindex_not_ready_exc_t) {
    assert_thread();

    /* Acquire the sindex block. */
    buf_lock_t sindex_block
        = acquire_sindex_block_for_read(superblock->expose_buf(),
                                        superblock->get_sindex_block_id());
    superblock->release();

    /* Figure out what the superblock for this index is. */
    secondary_index_t sindex;
    if (!::get_secondary_index(&sindex_block, id, &sindex)) {
        return false;
    }

    if (opaque_definition_out != NULL) {
        *opaque_definition_out = sindex.opaque_definition;
    }

    if (sindex_uuid_out != NULL) {
        *sindex_uuid_out = sindex.id;
    }

    if (!sindex.is_ready()) {
        throw sindex_not_ready_exc_t(id);
    }

    buf_lock_t superblock_lock(&sindex_block, sindex.superblock, access_t::read);
    sindex_block.reset_buf_lock();
    sindex_sb_out->init(new real_superblock_t(std::move(superblock_lock)));
    return true;
}

template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::acquire_sindex_superblock_for_write(
        const std::string &id,
        superblock_t *superblock,
        scoped_ptr_t<real_superblock_t> *sindex_sb_out,
        uuid_u *sindex_uuid_out)
    THROWS_ONLY(sindex_not_ready_exc_t) {
    assert_thread();

    /* Get the sindex block. */
    buf_lock_t sindex_block
        = acquire_sindex_block_for_write(superblock->expose_buf(),
                                         superblock->get_sindex_block_id());
    superblock->release();

    /* Figure out what the superblock for this index is. */
    secondary_index_t sindex;
    if (!::get_secondary_index(&sindex_block, id, &sindex)) {
        return false;
    }

    if (sindex_uuid_out != NULL) {
        *sindex_uuid_out = sindex.id;
    }

    if (!sindex.is_ready()) {
        throw sindex_not_ready_exc_t(id);
    }


    buf_lock_t superblock_lock(&sindex_block, sindex.superblock,
                               access_t::write);
    sindex_block.reset_buf_lock();
    sindex_sb_out->init(new real_superblock_t(std::move(superblock_lock)));
    return true;
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_all_sindex_superblocks_for_write(
        block_id_t sindex_block_id,
        buf_parent_t parent,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_ready_exc_t) {
    assert_thread();

    /* Get the sindex block. */
    buf_lock_t sindex_block = acquire_sindex_block_for_write(parent, sindex_block_id);

    acquire_all_sindex_superblocks_for_write(&sindex_block, sindex_sbs_out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_all_sindex_superblocks_for_write(
        buf_lock_t *sindex_block,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_ready_exc_t) {
    acquire_sindex_superblocks_for_write(
            boost::optional<std::set<std::string> >(),
            sindex_block,
            sindex_sbs_out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_post_constructed_sindex_superblocks_for_write(
        buf_lock_t *sindex_block,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_NOTHING {
    assert_thread();
    std::set<std::string> sindexes_to_acquire;
    std::map<std::string, secondary_index_t> sindexes;
    ::get_secondary_indexes(sindex_block, &sindexes);

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        /* Note that this can include indexes currently being deleted.
        In fact it must include those indexes if they had been post constructed
        before, since there might still be ongoing transactions traversing the
        index despite it being under deletion.*/
        if (it->second.post_construction_complete) {
            sindexes_to_acquire.insert(it->first);
        }
    }

    acquire_sindex_superblocks_for_write(
            sindexes_to_acquire, sindex_block,
            sindex_sbs_out);
}

template <class protocol_t>
bool btree_store_t<protocol_t>::acquire_sindex_superblocks_for_write(
            boost::optional<std::set<std::string> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_ready_exc_t) {
    assert_thread();

    std::map<std::string, secondary_index_t> sindexes;
    ::get_secondary_indexes(sindex_block, &sindexes);

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (sindexes_to_acquire && !std_contains(*sindexes_to_acquire, it->first)) {
            continue;
        }

        /* Getting the slice and asserting we're on the right thread. */
        btree_slice_t *sindex_slice = &(secondary_index_slices.at(it->second.id));
        sindex_slice->assert_thread();

        buf_lock_t superblock_lock(
                sindex_block, it->second.superblock, access_t::write);

        sindex_sbs_out->push_back(new
                sindex_access_t(get_sindex_slice(it->second.id), it->second, new
                    real_superblock_t(std::move(superblock_lock))));
    }

    //return's true if we got all of the sindexes requested.
    return sindex_sbs_out->size() == sindexes_to_acquire->size();
}

template <class protocol_t>
bool btree_store_t<protocol_t>::acquire_sindex_superblocks_for_write(
            boost::optional<std::set<uuid_u> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_ready_exc_t) {
    assert_thread();

    std::map<std::string, secondary_index_t> sindexes;
    ::get_secondary_indexes(sindex_block, &sindexes);

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (sindexes_to_acquire && !std_contains(*sindexes_to_acquire, it->second.id)) {
            continue;
        }

        /* Getting the slice and asserting we're on the right thread. */
        btree_slice_t *sindex_slice = &(secondary_index_slices.at(it->second.id));
        sindex_slice->assert_thread();

        buf_lock_t superblock_lock(
                sindex_block, it->second.superblock, access_t::write);

        sindex_sbs_out->push_back(new
                sindex_access_t(get_sindex_slice(it->second.id), it->second, new
                    real_superblock_t(std::move(superblock_lock))));
    }

    //return's true if we got all of the sindexes requested.
    return sindex_sbs_out->size() == sindexes_to_acquire->size();
}

template <class protocol_t>
void btree_store_t<protocol_t>::check_and_update_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
        const metainfo_t &new_metainfo,
        real_superblock_t *superblock) const
        THROWS_NOTHING {
    assert_thread();
    metainfo_t old_metainfo = check_metainfo(DEBUG_ONLY(metainfo_checker, ) superblock);
    update_metainfo(old_metainfo, new_metainfo, superblock);
}

template <class protocol_t>
typename btree_store_t<protocol_t>::metainfo_t
btree_store_t<protocol_t>::check_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
        real_superblock_t *superblock) const
        THROWS_NOTHING {
    assert_thread();
    region_map_t<protocol_t, binary_blob_t> old_metainfo;
    get_metainfo_internal(superblock->get(), &old_metainfo);
#ifndef NDEBUG
    metainfo_checker.check_metainfo(old_metainfo.mask(metainfo_checker.get_domain()));
#endif
    return old_metainfo;
}

template <class protocol_t>
void btree_store_t<protocol_t>::update_metainfo(const metainfo_t &old_metainfo,
                                                const metainfo_t &new_metainfo,
                                                real_superblock_t *superblock)
    const THROWS_NOTHING {
    assert_thread();
    region_map_t<protocol_t, binary_blob_t> updated_metadata = old_metainfo;
    updated_metadata.update(new_metainfo);

    rassert(updated_metadata.get_domain() == protocol_t::region_t::universe());

    buf_lock_t *sb_buf = superblock->get();
    clear_superblock_metainfo(sb_buf);

    for (typename region_map_t<protocol_t, binary_blob_t>::const_iterator i = updated_metadata.begin(); i != updated_metadata.end(); ++i) {
        vector_stream_t key;
        write_message_t msg;
        msg << i->first;
        key.reserve(msg.size());
        DEBUG_VAR int res = send_write_message(&key, &msg);
        rassert(!res);

        std::vector<char> value(static_cast<const char*>(i->second.data()),
                                static_cast<const char*>(i->second.data()) + i->second.size());

        set_superblock_metainfo(sb_buf, key.vector(), value); // FIXME: this is not efficient either, see how value is created
    }
}

template <class protocol_t>
void btree_store_t<protocol_t>::do_get_metainfo(UNUSED order_token_t order_token,  // TODO
                                                object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
                                                signal_t *interruptor,
                                                metainfo_t *out) THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_read(token,
                                &txn, &superblock,
                                interruptor,
                                false /* KSI: christ */);

    get_metainfo_internal(superblock->get(), out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::
get_metainfo_internal(buf_lock_t *sb_buf,
                      region_map_t<protocol_t, binary_blob_t> *out)
    const THROWS_NOTHING {
    assert_thread();
    std::vector<std::pair<std::vector<char>, std::vector<char> > > kv_pairs;
    // TODO: this is inefficient, cut out the middleman (vector)
    get_superblock_metainfo(sb_buf, &kv_pairs);

    std::vector<std::pair<typename protocol_t::region_t, binary_blob_t> > result;
    for (std::vector<std::pair<std::vector<char>, std::vector<char> > >::iterator i = kv_pairs.begin(); i != kv_pairs.end(); ++i) {
        const std::vector<char> &value = i->second;

        typename protocol_t::region_t region;
        {
            inplace_vector_read_stream_t key(&i->first);
            archive_result_t res = deserialize(&key, &region);
            guarantee_deserialization(res, "region");
        }

        result.push_back(std::make_pair(region, binary_blob_t(value.begin(), value.end())));
    }
    region_map_t<protocol_t, binary_blob_t> res(result.begin(), result.end());
    rassert(res.get_domain() == protocol_t::region_t::universe());
    *out = res;
}

template <class protocol_t>
void btree_store_t<protocol_t>::set_metainfo(const metainfo_t &new_metainfo,
                                             UNUSED order_token_t order_token,  // TODO
                                             object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
                                             signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    // KSI: Are there other places where we give up and use repli_timestamp_t::invalid?
    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_write(repli_timestamp_t::invalid,
                                 1,
                                 write_durability_t::HARD,
                                 token,
                                 &txn,
                                 &superblock,
                                 interruptor);

    region_map_t<protocol_t, binary_blob_t> old_metainfo;
    get_metainfo_internal(superblock->get(), &old_metainfo);
    update_metainfo(old_metainfo, new_metainfo, superblock.get());
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_read(
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
        scoped_ptr_t<txn_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor,
        bool use_snapshot)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(token);
    wait_interruptible(token->get(), interruptor);

    cache_snapshotted_t cache_snapshotted =
        use_snapshot ? CACHE_SNAPSHOTTED_YES : CACHE_SNAPSHOTTED_NO;
    get_btree_superblock_and_txn_for_reading(
        general_cache_conn.get(), cache_snapshotted, sb_out, txn_out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_backfill(
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
        scoped_ptr_t<txn_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(token);
    wait_interruptible(token->get(), interruptor);

    get_btree_superblock_and_txn_for_backfilling(general_cache_conn.get(),
                                                 btree->get_backfill_account(),
                                                 sb_out, txn_out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_write(
        repli_timestamp_t timestamp,
        int expected_change_count,
        const write_durability_t durability,
        write_token_pair_t *token_pair,
        scoped_ptr_t<txn_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    acquire_superblock_for_write(timestamp,
                                 expected_change_count, durability,
                                 &token_pair->main_write_token, txn_out, sb_out,
                                 interruptor);
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_write(
        repli_timestamp_t timestamp,
        int expected_change_count,
        write_durability_t durability,
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
        scoped_ptr_t<txn_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(token);
    wait_interruptible(token->get(), interruptor);

    get_btree_superblock_and_txn(general_cache_conn.get(), write_access_t::write,
                                 expected_change_count, timestamp,
                                 durability, sb_out, txn_out);
}

/* store_view_t interface */
template <class protocol_t>
void btree_store_t<protocol_t>::new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token_out) {
    assert_thread();
    fifo_enforcer_read_token_t token = main_token_source.enter_read();
    token_out->create(&main_token_sink, token);
}

template <class protocol_t>
void btree_store_t<protocol_t>::new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token_out) {
    assert_thread();
    fifo_enforcer_write_token_t token = main_token_source.enter_write();
    token_out->create(&main_token_sink, token);
}

#include "memcached/protocol.hpp"
template class btree_store_t<memcached_protocol_t>;

#include "rdb_protocol/protocol.hpp"
template class btree_store_t<rdb_protocol_t>;
