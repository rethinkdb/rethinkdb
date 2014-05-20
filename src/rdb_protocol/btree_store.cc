// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/store.hpp"

#include "btree/operations.hpp"
#include "btree/secondary_operations.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/cache_balancer.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/archive/versioned.hpp"
#include "containers/disk_backed_queue.hpp"
#include "rdb_protocol/protocol.hpp"
#include "serializer/config.hpp"
#include "stl_utils.hpp"

// Some of this implementation is in store.cc and some in btree_store.cc for no
// particularly good reason.  Historically it turned out that way, and for now
// there's not enough refactoring urgency to combine them into one.
//
// (Really, some of the implementation is also in rdb_protocol/btree.cc.)

sindex_not_post_constructed_exc_t::sindex_not_post_constructed_exc_t(
    const std::string &sindex_name, const std::string &table_name)
    : info(strprintf("Index `%s` on table `%s` "
                     "was accessed before its construction was finished.",
                     sindex_name.c_str(),
                     table_name.c_str())) { }

const char *sindex_not_post_constructed_exc_t::what() const throw() {
    return info.c_str();
}

sindex_not_post_constructed_exc_t::~sindex_not_post_constructed_exc_t() throw() { }

store_t::store_t(serializer_t *serializer,
                 cache_balancer_t *balancer,
                 const std::string &perfmon_name,
                 bool create,
                 perfmon_collection_t *parent_perfmon_collection,
                 rdb_context_t *_ctx,
                 io_backender_t *io_backender,
                 const base_path_t &base_path)
    : store_view_t(region_t::universe()),
      perfmon_collection(),
      io_backender_(io_backender), base_path_(base_path),
      perfmon_collection_membership(parent_perfmon_collection, &perfmon_collection, perfmon_name),
      ctx(_ctx)
{
    cache.init(new cache_t(serializer, balancer, &perfmon_collection));
    general_cache_conn.init(new cache_conn_t(cache.get()));

    if (create) {
        vector_stream_t key;
        // The version used when deserializing this data depends on the block magic.
        // The block magic set by init_superblock corresponds to the latest version
        // and so this serialization does too.
        // VSI: Do this better.
        write_message_t wm;
        region_t kr = region_t::universe();
        serialize_for_version(cluster_version_t::ONLY_VERSION, &wm, kr);
        key.reserve(wm.size());
        int res = send_write_message(&key, &wm);
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
        store_view_t::new_read_token_pair(&token_pair);

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
            secondary_index_slices.insert(it->first,
                                          new btree_slice_t(cache.get(),
                                                            &perfmon_collection,
                                                            it->first));
        }
    }

    help_construct_bring_sindexes_up_to_date();
}

store_t::~store_t() {
    assert_thread();
}

void store_t::read(
        DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
        const read_t &read,
        read_response_t *response,
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

    protocol_read(read, response, superblock.get(), interruptor);
}

void store_t::write(
        DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
        const metainfo_t& new_metainfo,
        const write_t &write,
        write_response_t *response,
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
    protocol_write(write, response, timestamp, &superblock, interruptor);
}

// TODO: Figure out wtf does the backfill filtering, figure out wtf constricts delete range operations to hit only a certain hash-interval, figure out what filters keys.
bool store_t::send_backfill(
        const region_map_t<state_timestamp_t> &start_point,
        send_backfill_callback_t *send_backfill_cb,
        traversal_progress_combiner_t *progress,
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

    region_map_t<binary_blob_t> unmasked_metainfo;
    get_metainfo_internal(superblock->get(), &unmasked_metainfo);
    region_map_t<binary_blob_t> metainfo = unmasked_metainfo.mask(start_point.get_domain());
    if (send_backfill_cb->should_backfill(metainfo)) {
        protocol_send_backfill(start_point, send_backfill_cb, superblock.get(), &sindex_block, progress, interruptor);
        return true;
    }
    return false;
}

void store_t::receive_backfill(
        const backfill_chunk_t &chunk,
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
    protocol_receive_backfill(std::move(superblock),
                              interruptor,
                              chunk);
}

void store_t::reset_data(
        const region_t &subregion,
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

    region_map_t<binary_blob_t> old_metainfo;
    get_metainfo_internal(superblock->get(), &old_metainfo);
    update_metainfo(old_metainfo, new_metainfo, superblock.get());

    protocol_reset_data(subregion,
                        superblock.get(),
                        interruptor);
}

scoped_ptr_t<new_mutex_in_line_t> store_t::get_in_line_for_sindex_queue(
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

void store_t::register_sindex_queue(
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

void store_t::deregister_sindex_queue(
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

void store_t::emergency_deregister_sindex_queue(
    internal_disk_backed_queue_t *disk_backed_queue) {
    assert_thread();
    drainer.assert_draining();
    new_mutex_in_line_t acq(&sindex_queue_mutex);
    acq.acq_signal()->wait_lazily_unordered();

    deregister_sindex_queue(disk_backed_queue, &acq);
}

void store_t::sindex_queue_push(const write_message_t &value,
                                const new_mutex_in_line_t *acq) {
    assert_thread();
    acq->acq_signal()->wait_lazily_unordered();

    for (auto it = sindex_queues.begin(); it != sindex_queues.end(); ++it) {
        (*it)->push(value);
    }
}

void store_t::sindex_queue_push(
        const scoped_array_t<write_message_t> &values,
        const new_mutex_in_line_t *acq) {
    assert_thread();
    acq->acq_signal()->wait_lazily_unordered();

    for (auto it = sindex_queues.begin(); it != sindex_queues.end(); ++it) {
        (*it)->push(values);
    }
}

void store_t::add_progress_tracker(
        map_insertion_sentry_t<uuid_u, const parallel_traversal_progress_t *> *sentry,
        uuid_u id, const parallel_traversal_progress_t *p) {
    assert_thread();
    sentry->reset(&progress_trackers, id, p);
}

progress_completion_fraction_t store_t::get_progress(uuid_u id) {
    if (!std_contains(progress_trackers, id)) {
        return progress_completion_fraction_t();
    } else {
        return progress_trackers[id]->guess_completion();
    }
}

// KSI: If we're going to have these functions at all, we could just pass the
// real_superblock_t directly.
buf_lock_t store_t::acquire_sindex_block_for_read(
        buf_parent_t parent,
        block_id_t sindex_block_id) {
    return buf_lock_t(parent, sindex_block_id, access_t::read);
}

buf_lock_t store_t::acquire_sindex_block_for_write(
        buf_parent_t parent,
        block_id_t sindex_block_id) {
    return buf_lock_t(parent, sindex_block_id, access_t::write);
}

bool store_t::add_sindex(
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
            id, new btree_slice_t(cache.get(), &perfmon_collection, id));

        sindex.post_construction_complete = false;

        ::set_secondary_index(sindex_block, id, sindex);
        return true;
    }
}

// KSI: There's no reason why this should work with detached sindexes.  Make this
// just take the buf_parent_t.  (The reason might be that it's interruptible?)
void clear_sindex(
        txn_t *txn, block_id_t superblock_id,
        value_sizer_t *sizer,
        const value_deleter_t *deleter, signal_t *interruptor) {
    /* Notice we're acquire sindex.superblock twice below which seems odd,
     * the reason for this is that erase_all releases the sindex_superblock
     * that we pass to it because that makes sense at other call sites.
     * Thus we need to reacquire it to delete it. */
    {
        /* We pass the txn as a parent, this is because
         * sindex.superblock was detached and thus now has no parent. */
        buf_lock_t sindex_superblock_lock(
                buf_parent_t(txn), superblock_id, access_t::write);
        real_superblock_t sindex_superblock(std::move(sindex_superblock_lock));

        erase_all(sizer,
                  deleter, &sindex_superblock, interruptor);
    }

    {
        buf_lock_t sindex_superblock_lock(
                buf_parent_t(txn), superblock_id, access_t::write);
        sindex_superblock_lock.mark_deleted();
    }
}

void store_t::set_sindexes(
        const std::map<std::string, secondary_index_t> &sindexes,
        buf_lock_t *sindex_block,
        value_sizer_t *sizer,
        const deletion_context_t *live_deletion_context,
        const deletion_context_t *post_construction_deletion_context,
        std::set<std::string> *created_sindexes_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {

    std::map<std::string, secondary_index_t> existing_sindexes;
    ::get_secondary_indexes(sindex_block, &existing_sindexes);

    // KSI: This is kind of malperformant because we call clear_sindex while holding
    // on to sindex_block.

    // KSI: This is malperformant because we don't parallelize this stuff.

    for (auto it = existing_sindexes.begin(); it != existing_sindexes.end(); ++it) {
        if (!std_contains(sindexes, it->first)) {
            delete_secondary_index(sindex_block, it->first);
            /* After deleting sindex from the sindex_block we can now detach it as
             * a child, it's now a parentless block. */
            sindex_block->detach_child(it->second.superblock);

            /* If the index had been completely constructed, we must detach
             * its values since snapshots might be accessing it.  If on the other
             * hand the index has not finished post construction, it would be
             * incorrect to do so. The reason being that some of the values that
             * the sindex points to might have been deleted in the meantime
             * (the deletion would be on the sindex queue, but might not have
             * found its way into the index tree yet). */
            const deletion_context_t *actual_deletion_context =
                    it->second.post_construction_complete
                    ? live_deletion_context
                    : post_construction_deletion_context;

            guarantee(std_contains(secondary_index_slices, it->first));
            clear_sindex(sindex_block->txn(), it->second.superblock,
                         sizer, actual_deletion_context->in_tree_deleter(),
                         interruptor);
            secondary_index_slices.erase(it->first);
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

            std::string id = it->first;
            secondary_index_slices.insert(id, new btree_slice_t(cache.get(), &perfmon_collection, it->first));

            sindex.post_construction_complete = false;

            ::set_secondary_index(sindex_block, it->first, sindex);

            created_sindexes_out->insert(it->first);
        }
    }
}

bool store_t::mark_index_up_to_date(const std::string &id,
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

bool store_t::mark_index_up_to_date(uuid_u id,
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

MUST_USE bool store_t::drop_sindex(
        const std::string &id,
        buf_lock_t *sindex_block,
        value_sizer_t *sizer,
        const deletion_context_t *live_deletion_context,
        const deletion_context_t *post_construction_deletion_context,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    /* Remove reference in the super block */
    secondary_index_t sindex;
    if (!::get_secondary_index(sindex_block, id, &sindex)) {
        return false;
    } else {
        // Similar to `store_t::set_sindexes()`, we have to pick a deletion
        // context based on whether the sindex had finished post construction or not.
        const deletion_context_t *actual_deletion_context =
                sindex.post_construction_complete
                ? live_deletion_context
                : post_construction_deletion_context;

        delete_secondary_index(sindex_block, id);
        /* After deleting sindex from the sindex_block we can now detach it as
         * a child, it's now a parentless block. */
        sindex_block->detach_child(sindex.superblock);
        /* We need to save the txn because we are about to release this block. */
        txn_t *txn = sindex_block->txn();
        /* Release the sindex block so others may proceed. */
        sindex_block->reset_buf_lock();

        /* Make sure we have a record of the slice. */
        guarantee(std_contains(secondary_index_slices, id));
        clear_sindex(txn, sindex.superblock,
                     sizer, actual_deletion_context->in_tree_deleter(), interruptor);
        secondary_index_slices.erase(id);
    }
    return true;
}

MUST_USE bool store_t::acquire_sindex_superblock_for_read(
        const std::string &id,
        const std::string &table_name,
        superblock_t *superblock,
        scoped_ptr_t<real_superblock_t> *sindex_sb_out,
        std::vector<char> *opaque_definition_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
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

    if (!sindex.post_construction_complete) {
        throw sindex_not_post_constructed_exc_t(id, table_name);
    }

    buf_lock_t superblock_lock(&sindex_block, sindex.superblock, access_t::read);
    sindex_block.reset_buf_lock();
    sindex_sb_out->init(new real_superblock_t(std::move(superblock_lock)));
    return true;
}

MUST_USE bool store_t::acquire_sindex_superblock_for_write(
        const std::string &id,
        const std::string &table_name,
        superblock_t *superblock,
        scoped_ptr_t<real_superblock_t> *sindex_sb_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
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

    if (!sindex.post_construction_complete) {
        throw sindex_not_post_constructed_exc_t(id, table_name);
    }


    buf_lock_t superblock_lock(&sindex_block, sindex.superblock,
                               access_t::write);
    sindex_block.reset_buf_lock();
    sindex_sb_out->init(new real_superblock_t(std::move(superblock_lock)));
    return true;
}

void store_t::acquire_all_sindex_superblocks_for_write(
        block_id_t sindex_block_id,
        buf_parent_t parent,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
    assert_thread();

    /* Get the sindex block. */
    buf_lock_t sindex_block = acquire_sindex_block_for_write(parent, sindex_block_id);

    acquire_all_sindex_superblocks_for_write(&sindex_block, sindex_sbs_out);
}

void store_t::acquire_all_sindex_superblocks_for_write(
        buf_lock_t *sindex_block,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
    acquire_sindex_superblocks_for_write(
            boost::optional<std::set<std::string> >(),
            sindex_block,
            sindex_sbs_out);
}

void store_t::acquire_post_constructed_sindex_superblocks_for_write(
        buf_lock_t *sindex_block,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_NOTHING {
    assert_thread();
    std::set<std::string> sindexes_to_acquire;
    std::map<std::string, secondary_index_t> sindexes;
    ::get_secondary_indexes(sindex_block, &sindexes);

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (it->second.post_construction_complete) {
            sindexes_to_acquire.insert(it->first);
        }
    }

    acquire_sindex_superblocks_for_write(
            sindexes_to_acquire, sindex_block,
            sindex_sbs_out);
}

bool store_t::acquire_sindex_superblocks_for_write(
            boost::optional<std::set<std::string> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
    assert_thread();

    std::map<std::string, secondary_index_t> sindexes;
    ::get_secondary_indexes(sindex_block, &sindexes);

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (sindexes_to_acquire && !std_contains(*sindexes_to_acquire, it->first)) {
            continue;
        }

        /* Getting the slice and asserting we're on the right thread. */
        btree_slice_t *sindex_slice = &(secondary_index_slices.at(it->first));
        sindex_slice->assert_thread();

        buf_lock_t superblock_lock(
                sindex_block, it->second.superblock, access_t::write);

        sindex_sbs_out->push_back(new
                sindex_access_t(get_sindex_slice(it->first), it->second, new
                    real_superblock_t(std::move(superblock_lock))));
    }

    //return's true if we got all of the sindexes requested.
    return sindex_sbs_out->size() == sindexes_to_acquire->size();
}

bool store_t::acquire_sindex_superblocks_for_write(
            boost::optional<std::set<uuid_u> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
    assert_thread();

    std::map<std::string, secondary_index_t> sindexes;
    ::get_secondary_indexes(sindex_block, &sindexes);

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (sindexes_to_acquire && !std_contains(*sindexes_to_acquire, it->second.id)) {
            continue;
        }

        /* Getting the slice and asserting we're on the right thread. */
        btree_slice_t *sindex_slice = &(secondary_index_slices.at(it->first));
        sindex_slice->assert_thread();

        buf_lock_t superblock_lock(
                sindex_block, it->second.superblock, access_t::write);

        sindex_sbs_out->push_back(new
                sindex_access_t(get_sindex_slice(it->first), it->second, new
                    real_superblock_t(std::move(superblock_lock))));
    }

    //return's true if we got all of the sindexes requested.
    return sindex_sbs_out->size() == sindexes_to_acquire->size();
}

void store_t::check_and_update_metainfo(
        DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
        const metainfo_t &new_metainfo,
        real_superblock_t *superblock) const
        THROWS_NOTHING {
    assert_thread();
    metainfo_t old_metainfo = check_metainfo(DEBUG_ONLY(metainfo_checker, ) superblock);
    update_metainfo(old_metainfo, new_metainfo, superblock);
}

metainfo_t
store_t::check_metainfo(
        DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
        real_superblock_t *superblock) const
        THROWS_NOTHING {
    assert_thread();
    region_map_t<binary_blob_t> old_metainfo;
    get_metainfo_internal(superblock->get(), &old_metainfo);
#ifndef NDEBUG
    metainfo_checker.check_metainfo(old_metainfo.mask(metainfo_checker.get_domain()));
#endif
    return old_metainfo;
}

void store_t::update_metainfo(const metainfo_t &old_metainfo,
                              const metainfo_t &new_metainfo,
                              real_superblock_t *superblock)
    const THROWS_NOTHING {
    assert_thread();
    region_map_t<binary_blob_t> updated_metadata = old_metainfo;
    updated_metadata.update(new_metainfo);

    rassert(updated_metadata.get_domain() == region_t::universe());

    buf_lock_t *sb_buf = superblock->get();
    clear_superblock_metainfo(sb_buf);

    for (region_map_t<binary_blob_t>::const_iterator i = updated_metadata.begin(); i != updated_metadata.end(); ++i) {
        vector_stream_t key;
        write_message_t wm;
        // Versioning of this serialization will depend on the block magic.  But
        // right now there's just one version.
        // VSI: Make this code better.
        serialize_for_version(cluster_version_t::ONLY_VERSION, &wm, i->first);
        key.reserve(wm.size());
        DEBUG_VAR int res = send_write_message(&key, &wm);
        rassert(!res);

        std::vector<char> value(static_cast<const char*>(i->second.data()),
                                static_cast<const char*>(i->second.data()) + i->second.size());

        set_superblock_metainfo(sb_buf, key.vector(), value); // FIXME: this is not efficient either, see how value is created
    }
}

void store_t::do_get_metainfo(UNUSED order_token_t order_token,  // TODO
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

void store_t::
get_metainfo_internal(buf_lock_t *sb_buf,
                      region_map_t<binary_blob_t> *out)
    const THROWS_NOTHING {
    assert_thread();
    std::vector<std::pair<std::vector<char>, std::vector<char> > > kv_pairs;
    // TODO: this is inefficient, cut out the middleman (vector)
    get_superblock_metainfo(sb_buf, &kv_pairs);

    std::vector<std::pair<region_t, binary_blob_t> > result;
    for (std::vector<std::pair<std::vector<char>, std::vector<char> > >::iterator i = kv_pairs.begin(); i != kv_pairs.end(); ++i) {
        const std::vector<char> &value = i->second;

        region_t region;
        {
            inplace_vector_read_stream_t key(&i->first);
            // Versioning of this deserialization will depend on the block magic.
            // VSI: Make this code better.
            archive_result_t res = deserialize_for_version(cluster_version_t::ONLY_VERSION, &key, &region);
            guarantee_deserialization(res, "region");
        }

        result.push_back(std::make_pair(region, binary_blob_t(value.begin(), value.end())));
    }
    region_map_t<binary_blob_t> res(result.begin(), result.end());
    rassert(res.get_domain() == region_t::universe());
    *out = res;
}

void store_t::set_metainfo(const metainfo_t &new_metainfo,
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

    region_map_t<binary_blob_t> old_metainfo;
    get_metainfo_internal(superblock->get(), &old_metainfo);
    update_metainfo(old_metainfo, new_metainfo, superblock.get());
}

void store_t::acquire_superblock_for_read(
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

void store_t::acquire_superblock_for_backfill(
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

void store_t::acquire_superblock_for_write(
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

void store_t::acquire_superblock_for_write(
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
void store_t::new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token_out) {
    assert_thread();
    fifo_enforcer_read_token_t token = main_token_source.enter_read();
    token_out->create(&main_token_sink, token);
}

void store_t::new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token_out) {
    assert_thread();
    fifo_enforcer_write_token_t token = main_token_source.enter_write();
    token_out->create(&main_token_sink, token);
}
