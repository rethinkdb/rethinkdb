// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "btree/btree_store.hpp"

#include "btree/operations.hpp"
#include "btree/secondary_operations.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/vector_stream.hpp"
#include "serializer/config.hpp"
#include "stl_utils.hpp"

#if SLICE_ALT
using alt::alt_access_t;
using alt::alt_buf_lock_t;
using alt::alt_buf_parent_t;
using alt::alt_cache_t;
using alt::alt_create_t;
using alt::alt_txn_t;
#endif

sindex_not_post_constructed_exc_t::sindex_not_post_constructed_exc_t(
        std::string sindex_name)
    : info(strprintf("Sindex: %s was accessed before it was finished post constructing.",
                sindex_name.c_str()))
{ }

const char* sindex_not_post_constructed_exc_t::what() const throw() {
    return info.c_str();
}

sindex_not_post_constructed_exc_t::~sindex_not_post_constructed_exc_t() throw() { }

template <class protocol_t>
btree_store_t<protocol_t>::btree_store_t(serializer_t *serializer,
                                         const std::string &perfmon_name,
                                         int64_t cache_target,
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
    debugf("btree_store_t constructor\n");
    if (create) {
        cache_t::create(serializer);
    }

    // TODO: Don't specify cache dynamic config here.
    // RSI: Remove or replace cache_dynamic_config.
    cache_dynamic_config.max_size = cache_target;
    cache_dynamic_config.max_dirty_size = cache_target / 2;
#if SLICE_ALT
    cache.init(new alt_cache_t(serializer));
#else
    cache.init(new cache_t(serializer, cache_dynamic_config, &perfmon_collection));
#endif

    if (create) {
        vector_stream_t key;
        write_message_t msg;
        typename protocol_t::region_t kr = protocol_t::region_t::universe();
        msg << kr;
        int res = send_write_message(&key, &msg);
        guarantee(!res);

        btree_slice_t::create(cache.get(), key.vector(), std::vector<char>());
    }

    btree.init(new btree_slice_t(cache.get(), &perfmon_collection, "primary"));

    // Initialize sindex slices
    {
        // Since this is the btree constructor, nothing else should be locking these
        //  things yet, so this should work fairly quickly and does not need a real
        //  interruptor
        cond_t dummy_interruptor;
        read_token_pair_t token_pair;
        new_read_token_pair(&token_pair);

#if SLICE_ALT
        scoped_ptr_t<alt_txn_t> txn;
#else
        scoped_ptr_t<transaction_t> txn;
#endif
        scoped_ptr_t<real_superblock_t> superblock;
#if SLICE_ALT
        acquire_superblock_for_read(&token_pair.main_read_token, &txn,
                                    &superblock, &dummy_interruptor, false);
#else
        acquire_superblock_for_read(rwi_read, &token_pair.main_read_token, &txn,
                                    &superblock, &dummy_interruptor, false);
#endif

#if SLICE_ALT
        scoped_ptr_t<alt_buf_lock_t> sindex_block;
        // RSI: Before we passed a token pair and made acquiring the sindex block
        // interruptible.  Do any callers not pass a dummy interruptor?
        acquire_sindex_block_for_read(superblock->expose_buf(),
                                      &sindex_block,
                                      superblock->get_sindex_block_id(),
                                      &dummy_interruptor);
#else
        scoped_ptr_t<buf_lock_t> sindex_block;
        acquire_sindex_block_for_read(&token_pair, txn.get(), &sindex_block,
                                      superblock->get_sindex_block_id(),
                                      &dummy_interruptor);
#endif

        std::map<std::string, secondary_index_t> sindexes;
#if SLICE_ALT
        get_secondary_indexes(sindex_block.get(), &sindexes);
#else
        get_secondary_indexes(txn.get(), sindex_block.get(), &sindexes);
#endif

        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            secondary_index_slices.insert(it->first,
                                          new btree_slice_t(cache.get(),
                                                            &perfmon_collection,
                                                            it->first));
        }
    }
    debugf("btree_store_t constructor exit\n");
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
    debugf_t eex("%p read (%p)", this, response);

    assert_thread();
#if SLICE_ALT
    scoped_ptr_t<alt_txn_t> txn;
#else
    scoped_ptr_t<transaction_t> txn;
#endif
    scoped_ptr_t<real_superblock_t> superblock;

#if SLICE_ALT
    acquire_superblock_for_read(&token_pair->main_read_token, &txn, &superblock,
                                interruptor,
                                read.use_snapshot());
#else
    acquire_superblock_for_read(rwi_read, &token_pair->main_read_token, &txn, &superblock, interruptor,
                                read.use_snapshot());
#endif
    debugf("%p read (%p) acq superblock\n", this, response);

#if SLICE_ALT
    DEBUG_ONLY(check_metainfo(DEBUG_ONLY(metainfo_checker, ) superblock.get());)
#else
    DEBUG_ONLY(check_metainfo(DEBUG_ONLY(metainfo_checker, ) txn.get(), superblock.get());)
#endif
    debugf("%p read (%p) checked metainfo\n", this, response);

    // RSI: how is this an ugly hack
    // Ugly hack
    scoped_ptr_t<superblock_t> superblock2;
    superblock2.init(superblock.release());

#if SLICE_ALT
    protocol_read(read, response, btree.get(), superblock2.get(), interruptor);
#else
    protocol_read(read, response, btree.get(), txn.get(), superblock2.get(), token_pair, interruptor);
#endif
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
    debugf_t eex("%p write", this);

    assert_thread();

#if SLICE_ALT
    scoped_ptr_t<alt_txn_t> txn;
#else
    scoped_ptr_t<transaction_t> txn;
#endif
    scoped_ptr_t<real_superblock_t> real_superblock;
    const int expected_change_count = 2; // FIXME: this is incorrect, but will do for now
    acquire_superblock_for_write(timestamp.to_repli_timestamp(),
                                 expected_change_count, durability, token_pair,
                                 &txn, &real_superblock, interruptor);

#if SLICE_ALT
    check_and_update_metainfo(DEBUG_ONLY(metainfo_checker, ) new_metainfo,
                              real_superblock.get());
#else
    check_and_update_metainfo(DEBUG_ONLY(metainfo_checker, ) new_metainfo, txn.get(), real_superblock.get());
#endif
    scoped_ptr_t<superblock_t> superblock(real_superblock.release());
#if SLICE_ALT
    protocol_write(write, response, timestamp, btree.get(), &superblock,
                   interruptor);
#else
    protocol_write(write, response, timestamp, btree.get(), txn.get(), &superblock, token_pair, interruptor);
#endif
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
    debugf_t eex("%p send_backfill", this);

    assert_thread();

#if SLICE_ALT
    scoped_ptr_t<alt_txn_t> txn;
#else
    scoped_ptr_t<transaction_t> txn;
#endif
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_backfill(&token_pair->main_read_token, &txn, &superblock, interruptor);

#if SLICE_ALT
    scoped_ptr_t<alt_buf_lock_t> sindex_block;
#else
    scoped_ptr_t<buf_lock_t> sindex_block;
#endif
#if SLICE_ALT
    // RSI: We don't actually use interruptor in the called function anymore.
    acquire_sindex_block_for_read(superblock->expose_buf(),
                                  &sindex_block, superblock->get_sindex_block_id(),
                                  interruptor);
#else
    acquire_sindex_block_for_read(token_pair, txn.get(),
                                  &sindex_block, superblock->get_sindex_block_id(),
                                  interruptor);
#endif

    region_map_t<protocol_t, binary_blob_t> unmasked_metainfo;
#if SLICE_ALT
    get_metainfo_internal(superblock->get(), &unmasked_metainfo);
#else
    get_metainfo_internal(txn.get(), superblock->get(), &unmasked_metainfo);
#endif
    region_map_t<protocol_t, binary_blob_t> metainfo = unmasked_metainfo.mask(start_point.get_domain());
    if (send_backfill_cb->should_backfill(metainfo)) {
#if SLICE_ALT
        protocol_send_backfill(start_point, send_backfill_cb, superblock.get(), sindex_block.get(), btree.get(), progress, interruptor);
#else
        protocol_send_backfill(start_point, send_backfill_cb, superblock.get(), sindex_block.get(), btree.get(), txn.get(), progress, interruptor);
#endif
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
    debugf_t eex("%p receive_backfill", this);

    assert_thread();

#if SLICE_ALT
    scoped_ptr_t<alt_txn_t> txn;
#else
    scoped_ptr_t<transaction_t> txn;
#endif
    scoped_ptr_t<real_superblock_t> superblock;
    const int expected_change_count = 1; // FIXME: this is probably not correct

    // RSI: Make sure that protocol_receive_backfill acquires the secondary index
    // block at the right time.
#if !SLICE_ALT
    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t
        token_destroyer(&token_pair->sindex_write_token);
#endif

    // We don't want hard durability, this is a backfill chunk, and nobody
    // wants chunk-by-chunk acks.
    acquire_superblock_for_write(chunk.get_btree_repli_timestamp(),
                                 expected_change_count,
                                 write_durability_t::SOFT,
                                 token_pair,
                                 &txn,
                                 &superblock,
                                 interruptor);

    protocol_receive_backfill(btree.get(),
#if !SLICE_ALT
                              txn.get(),
#endif
                              superblock.get(),
#if !SLICE_ALT
                              token_pair,
#endif
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
    debugf_t eex("%p reset_data", this);

    assert_thread();

#if SLICE_ALT
    scoped_ptr_t<alt_txn_t> txn;
#else
    scoped_ptr_t<transaction_t> txn;
#endif
    scoped_ptr_t<real_superblock_t> superblock;

    // RSI: Make sure protocol_reset_data acquires the secondary index block at the
    // right time.
#if !SLICE_ALT
    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t
        token_destroyer(&token_pair->sindex_write_token);
#endif

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
#if SLICE_ALT
    get_metainfo_internal(superblock->get(), &old_metainfo);
#else
    get_metainfo_internal(txn.get(), superblock->get(), &old_metainfo);
#endif
#if SLICE_ALT
    update_metainfo(old_metainfo, new_metainfo, superblock.get());
#else
    update_metainfo(old_metainfo, new_metainfo, txn.get(), superblock.get());
#endif

    protocol_reset_data(subregion,
                        btree.get(),
#if !SLICE_ALT
                        txn.get(),
#endif
                        superblock.get(),
#if !SLICE_ALT
                        token_pair,
#endif
                        interruptor);
}

#if SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::lock_sindex_queue(alt_buf_lock_t *sindex_block,
                                                  mutex_t::acq_t *acq) {
#else
template <class protocol_t>
void btree_store_t<protocol_t>::lock_sindex_queue(buf_lock_t *sindex_block, mutex_t::acq_t *acq) {
#endif
    assert_thread();
#if SLICE_ALT
    // RSI: WTF should we do here?  Why is there a mutex?

    // RSI: Do we really need to wait for write acquisition?

    // RSI: Should we be able to "get in line" for the mutex and release the sindex
    // block or something?
    guarantee(!sindex_block->empty());
    sindex_block->write_acq_signal()->wait();
#else
    guarantee(sindex_block->is_acquired());
#endif
    acq->reset(&sindex_queue_mutex);
}

template <class protocol_t>
void btree_store_t<protocol_t>::register_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue,
            const mutex_t::acq_t *acq) {
    assert_thread();
    acq->assert_is_holding(&sindex_queue_mutex);

    for (std::vector<internal_disk_backed_queue_t *>::iterator it = sindex_queues.begin();
            it != sindex_queues.end(); ++it) {
        guarantee(*it != disk_backed_queue);
    }
    sindex_queues.push_back(disk_backed_queue);
}

template <class protocol_t>
void btree_store_t<protocol_t>::deregister_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue,
            const mutex_t::acq_t *acq) {
    assert_thread();
    acq->assert_is_holding(&sindex_queue_mutex);

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
    mutex_t::acq_t acq(&sindex_queue_mutex);

    deregister_sindex_queue(disk_backed_queue, &acq);
}

template <class protocol_t>
void btree_store_t<protocol_t>::sindex_queue_push(const write_message_t &value,
                                                  const mutex_t::acq_t *acq) {
    assert_thread();
    acq->assert_is_holding(&sindex_queue_mutex);

    for (auto it = sindex_queues.begin(); it != sindex_queues.end(); ++it) {
        (*it)->push(value);
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

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_sindex_block_for_read(
#if !SLICE_ALT
        read_token_pair_t *token_pair,
#endif
#if SLICE_ALT
        alt_buf_parent_t parent,
        scoped_ptr_t<alt_buf_lock_t> *sindex_block_out,
#else
        transaction_t *txn,
        scoped_ptr_t<buf_lock_t> *sindex_block_out,
#endif
        block_id_t sindex_block_id,
#if SLICE_ALT
        UNUSED signal_t *interruptor)  // RSI: Before, interruptor was used when
                                       // waiting for the sindex_read_token.  Right
                                       // here, there's no way to interrupt trying to
                                       // acquire a block.
#else
        signal_t *interruptor)
#endif
    THROWS_ONLY(interrupted_exc_t) {
#if !SLICE_ALT
    /* First wait for our turn. */
    wait_interruptible(token_pair->sindex_read_token.get(), interruptor);

    /* Make sure others will be allowed to proceed after this function is
     * completes. */
    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(&token_pair->sindex_read_token);
#endif

    /* Finally acquire the block. */
#if SLICE_ALT
    sindex_block_out->init(new alt_buf_lock_t(parent, sindex_block_id,
                                              alt_access_t::read));
#else
    sindex_block_out->init(new buf_lock_t(txn, sindex_block_id, rwi_read));
#endif
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_sindex_block_for_write(
#if !SLICE_ALT
        write_token_pair_t *token_pair,
#endif
#if SLICE_ALT
        alt_buf_parent_t parent,
        scoped_ptr_t<alt_buf_lock_t> *sindex_block_out,
#else
        transaction_t *txn,
        scoped_ptr_t<buf_lock_t> *sindex_block_out,
#endif
        block_id_t sindex_block_id,
#if SLICE_ALT
        UNUSED signal_t *interruptor)  // RSI: Before, interruptor was used when
                                       // waiting for the sindex_write_token.  Right
                                       // here, there's no way to interrupt trying to
                                       // acquire a block.  That happens elsewhere.
#else
        signal_t *interruptor)
#endif
    THROWS_ONLY(interrupted_exc_t) {

#if !SLICE_ALT
    /* First wait for our turn. */
    wait_interruptible(token_pair->sindex_write_token.get(), interruptor);

    /* Make sure others will be allowed to proceed after this function is
     * completes. */
    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(&token_pair->sindex_write_token);
#endif

    /* Finally acquire the block. */
#if SLICE_ALT
    sindex_block_out->init(new alt_buf_lock_t(parent, sindex_block_id,
                                              alt_access_t::write));
#else
    sindex_block_out->init(new buf_lock_t(txn, sindex_block_id, rwi_write));
#endif
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

#if !SLICE_ALT
template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::add_sindex(
        write_token_pair_t *token_pair,
        const std::string &id,
        const secondary_index_t::opaque_definition_t &definition,
        transaction_t *txn,
        superblock_t *super_block,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    /* Get the sindex block which we will need to modify. */
    scoped_ptr_t<buf_lock_t> sindex_block;

    return add_sindex(token_pair, id, definition, txn,
                      super_block, &sindex_block,
                      interruptor);
}
#endif

#if SLICE_ALT
template <class protocol_t>
bool btree_store_t<protocol_t>::add_sindex(
        const std::string &id,
        const secondary_index_t::opaque_definition_t &definition,
        alt_buf_lock_t *sindex_block,
        UNUSED signal_t *interruptor)  // RSI: unused
    THROWS_ONLY(interrupted_exc_t) {

    secondary_index_t sindex;
    if (::get_secondary_index(sindex_block, id, &sindex)) {
        return false; // sindex was already created
    } else {
        {
            alt_buf_lock_t sindex_superblock(sindex_block,
                                             alt_create_t::create);
            sindex.superblock = sindex_superblock.get_block_id();
            /* The buf lock is destroyed here which is important becase it allows
             * us to reacquire later when we make a btree_store. */
        }

        sindex.opaque_definition = definition;

        /* Notice we're passing in empty strings for metainfo. The metainfo in
         * the sindexes isn't used for anything but this could perhaps be
         * something that would give a better error if someone did try to use
         * it... on the other hand this code isn't exactly idiot proof even
         * with that. */
        btree_slice_t::create(sindex.superblock,
                              alt_buf_parent_t(sindex_block),
                              std::vector<char>(), std::vector<char>());
        secondary_index_slices.insert(
            id, new btree_slice_t(cache.get(), &perfmon_collection, id));

        sindex.post_construction_complete = false;

        ::set_secondary_index(sindex_block, id, sindex);
        return true;
    }
}
#endif

#if !SLICE_ALT
template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::add_sindex(
        write_token_pair_t *token_pair,
        const std::string &id,
        const secondary_index_t::opaque_definition_t &definition,
        transaction_t *txn,
        superblock_t *super_block,
        scoped_ptr_t<buf_lock_t> *sindex_block_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {

    assert_thread();

    acquire_sindex_block_for_write(token_pair, txn, sindex_block_out,
                                   super_block->get_sindex_block_id(), interruptor);

    secondary_index_t sindex;
    if (::get_secondary_index(txn, sindex_block_out->get(), id, &sindex)) {
        return false; // sindex was already created
    } else {
        {
            buf_lock_t sindex_superblock(txn);
            sindex.superblock = sindex_superblock.get_block_id();
            /* The buf lock is destroyed here which is important becase it allows
             * us to reacquire later when we make a btree_store. */
        }

        sindex.opaque_definition = definition;

        /* Notice we're passing in empty strings for metainfo. The metainfo in
         * the sindexes isn't used for anything but this could perhaps be
         * something that would give a better error if someone did try to use
         * it... on the other hand this code isn't exactly idiot proof even
         * with that. */
        btree_slice_t::create(txn->get_cache(), sindex.superblock,
                              txn, std::vector<char>(), std::vector<char>());
        secondary_index_slices.insert(
            id, new btree_slice_t(cache.get(), &perfmon_collection, id));

        sindex.post_construction_complete = false;

        ::set_secondary_index(txn, sindex_block_out->get(), id, sindex);
        return true;
    }
}
#endif

#if SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::set_sindexes(
        const std::map<std::string, secondary_index_t> &sindexes,
        alt::alt_buf_lock_t *sindex_block,
        value_sizer_t<void> *sizer,
        value_deleter_t *deleter,
        std::set<std::string> *created_sindexes_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {

    std::map<std::string, secondary_index_t> existing_sindexes;
    ::get_secondary_indexes(sindex_block, &existing_sindexes);

    for (auto it = existing_sindexes.begin(); it != existing_sindexes.end(); ++it) {
        if (!std_contains(sindexes, it->first)) {
            delete_secondary_index(sindex_block, it->first);

            guarantee(std_contains(secondary_index_slices, it->first));
            btree_slice_t *sindex_slice = &(secondary_index_slices.at(it->first));

            {
                alt_buf_lock_t sindex_superblock_lock(sindex_block,
                                                      it->second.superblock,
                                                      alt_access_t::write);
                real_superblock_t sindex_superblock(&sindex_superblock_lock);

                erase_all(sizer, sindex_slice,
                          deleter, &sindex_superblock,
                          interruptor);
            }

            secondary_index_slices.erase(it->first);

            {
                alt_buf_lock_t sindex_superblock_lock(sindex_block,
                                                      it->second.superblock,
                                                      alt_access_t::write);
                sindex_superblock_lock.mark_deleted();
            }
        }
    }

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (!std_contains(existing_sindexes, it->first)) {
            secondary_index_t sindex(it->second);
            {
                alt_buf_lock_t sindex_superblock(sindex_block,
                                                 alt_create_t::create);
                sindex.superblock = sindex_superblock.get_block_id();
                /* The buf lock is destroyed here which is important becase it allows
                 * us to reacquire later when we make a btree_store. */
            }

            btree_slice_t::create(sindex.superblock,
                                  alt_buf_parent_t(sindex_block),
                                  std::vector<char>(), std::vector<char>());
            std::string id = it->first;
            secondary_index_slices.insert(id, new btree_slice_t(cache.get(), &perfmon_collection, it->first));

            sindex.post_construction_complete = false;

            ::set_secondary_index(sindex_block, it->first, sindex);

            created_sindexes_out->insert(it->first);
        }
    }
}

#endif

#if !SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::set_sindexes(
        write_token_pair_t *token_pair,
        const std::map<std::string, secondary_index_t> &sindexes,
        transaction_t *txn,
        superblock_t *superblock,
        value_sizer_t<void> *sizer,
        value_deleter_t *deleter,
        scoped_ptr_t<buf_lock_t> *sindex_block_out,
        std::set<std::string> *created_sindexes_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    /* Get the sindex block which we will need to modify. */
    acquire_sindex_block_for_write(token_pair, txn, sindex_block_out, superblock->get_sindex_block_id(), interruptor);

    std::map<std::string, secondary_index_t> existing_sindexes;
    ::get_secondary_indexes(txn, sindex_block_out->get(), &existing_sindexes);

    for (auto it = existing_sindexes.begin(); it != existing_sindexes.end(); ++it) {
        if (!std_contains(sindexes, it->first)) {
            delete_secondary_index(txn, sindex_block_out->get(), it->first);

            guarantee(std_contains(secondary_index_slices, it->first));
            btree_slice_t *sindex_slice = &(secondary_index_slices.at(it->first));

            {
                buf_lock_t sindex_superblock_lock(txn, it->second.superblock, rwi_write);
                real_superblock_t sindex_superblock(&sindex_superblock_lock);

                erase_all(sizer, sindex_slice,
                          deleter, txn, &sindex_superblock,
                          interruptor);
            }

            secondary_index_slices.erase(it->first);

            {
                buf_lock_t sindex_superblock_lock(txn, it->second.superblock, rwi_write);
                sindex_superblock_lock.mark_deleted();
            }
        }
    }

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (!std_contains(existing_sindexes, it->first)) {
            secondary_index_t sindex(it->second);
            {
                buf_lock_t sindex_superblock(txn);
                sindex.superblock = sindex_superblock.get_block_id();
                /* The buf lock is destroyed here which is important becase it allows
                 * us to reacquire later when we make a btree_store. */
            }

            btree_slice_t::create(txn->get_cache(), sindex.superblock, txn,
                    std::vector<char>(), std::vector<char>());
            std::string id = it->first;
            secondary_index_slices.insert(id, new btree_slice_t(cache.get(), &perfmon_collection, it->first));

            sindex.post_construction_complete = false;

            ::set_secondary_index(txn, sindex_block_out->get(), it->first, sindex);

            created_sindexes_out->insert(it->first);
        }
    }
}
#endif

#if SLICE_ALT
template <class protocol_t>
bool btree_store_t<protocol_t>::mark_index_up_to_date(const std::string &id,
                                                      alt_buf_lock_t *sindex_block)
    THROWS_NOTHING {
#else
template <class protocol_t>
bool btree_store_t<protocol_t>::mark_index_up_to_date(
    const std::string &id,
    transaction_t *txn,
    buf_lock_t *sindex_block)
THROWS_NOTHING {
#endif
    secondary_index_t sindex;
#if SLICE_ALT
    bool found = ::get_secondary_index(sindex_block, id, &sindex);
#else
    bool found = ::get_secondary_index(txn, sindex_block, id, &sindex);
#endif

    if (found) {
        sindex.post_construction_complete = true;

#if SLICE_ALT
        ::set_secondary_index(sindex_block, id, sindex);
#else
        ::set_secondary_index(txn, sindex_block, id, sindex);
#endif
    }

    return found;
}

#if SLICE_ALT
template <class protocol_t>
bool btree_store_t<protocol_t>::mark_index_up_to_date(uuid_u id,
                                                      alt_buf_lock_t *sindex_block)
    THROWS_NOTHING {
#else
template <class protocol_t>
bool btree_store_t<protocol_t>::mark_index_up_to_date(
    uuid_u id,
    transaction_t *txn,
    buf_lock_t *sindex_block)
THROWS_NOTHING {
#endif
    secondary_index_t sindex;
#if SLICE_ALT
    bool found = ::get_secondary_index(sindex_block, id, &sindex);
#else
    bool found = ::get_secondary_index(txn, sindex_block, id, &sindex);
#endif

    if (found) {
        sindex.post_construction_complete = true;

#if SLICE_ALT
        ::set_secondary_index(sindex_block, id, sindex);
#else
        ::set_secondary_index(txn, sindex_block, id, sindex);
#endif
    }

    return found;
}

#if SLICE_ALT
template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::drop_sindex(
        const std::string &id,
        alt_buf_lock_t *sindex_block,
        value_sizer_t<void> *sizer,
        value_deleter_t *deleter,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    /* Remove reference in the super block */
    secondary_index_t sindex;
    if (!::get_secondary_index(sindex_block, id, &sindex)) {
        return false;
    } else {
        delete_secondary_index(sindex_block, id);
        // RSI: Ahem?  We can't release sindex_block here with the alt cache -- since
        // we re-acquire the sindex_superblock_lock over and over again.  Why do we
        // re-acquire the sindex_superblock_lock over and over again?

        // RSI: Erm -- we also can't release it because we don't own it.  Mybe talk
        // to the caller about releasing it.

        // RSI: (We MUST release the sindex block right away so that others can
        // proceed, yes.)

        // RSI: Does the cache let us mark a parent deleted and then acquire the
        // child of that block?  Logically it seems that once you've "detached" the
        // child from the parent, you could just acquire the child with the txn as
        // parent.

        /* Make sure we have a record of the slice. */
        guarantee(std_contains(secondary_index_slices, id));
        btree_slice_t *sindex_slice = &(secondary_index_slices.at(id));

        {
            alt_buf_lock_t sindex_superblock_lock(sindex_block,
                                                  sindex.superblock,
                                                  alt_access_t::write);
            real_superblock_t sindex_superblock(&sindex_superblock_lock);

            erase_all(sizer, sindex_slice,
                      deleter, &sindex_superblock, interruptor);
        }

        secondary_index_slices.erase(id);

        {
            alt_buf_lock_t sindex_superblock_lock(sindex_block,
                                                  sindex.superblock,
                                                  alt_access_t::write);
            sindex_superblock_lock.mark_deleted();
        }
        // RSI: Yeah, we don't want to do this here, thanks.
        // RSI: We don't do this at all because it's the caller's block to delete, I think.
        // sindex_block->reset_buf_lock();
    }
    return true;
}
#else
template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::drop_sindex(
        write_token_pair_t *token_pair,
        const std::string &id,
        transaction_t *txn,
        superblock_t *super_block,
        value_sizer_t<void> *sizer,
        value_deleter_t *deleter,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    /* First get the sindex block. */
    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_write(token_pair, txn, &sindex_block, super_block->get_sindex_block_id(), interruptor);

    /* Remove reference in the super block */
    secondary_index_t sindex;
    if (!::get_secondary_index(txn, sindex_block.get(), id, &sindex)) {
        return false;
    } else {
        delete_secondary_index(txn, sindex_block.get(), id);
        // RSI: Ahem?  We can't release sindex_block here with the alt cache -- since
        // we re-acquire the sindex_superblock_lock over and over again.  Why do we
        // re-acquire the sindex_superblock_lock over and over again?

        // RSI: (We MUST release the sindex block right away so that others can
        // proceed, yes.)

        // RSI: Does the cache let us mark a parent deleted and then acquire the
        // child of that block?  Logically it seems that once you've "detached" the
        // child from the parent, you could just acquire the child with the txn as
        // parent.
        sindex_block->release(); //So others may proceed

        /* Make sure we have a record of the slice. */
        guarantee(std_contains(secondary_index_slices, id));
        btree_slice_t *sindex_slice = &(secondary_index_slices.at(id));

        {
            buf_lock_t sindex_superblock_lock(txn, sindex.superblock, rwi_write);
            real_superblock_t sindex_superblock(&sindex_superblock_lock);

            erase_all(sizer, sindex_slice,
                      deleter, txn, &sindex_superblock, interruptor);
        }

        secondary_index_slices.erase(id);

        {
            buf_lock_t sindex_superblock_lock(txn, sindex.superblock, rwi_write);
            sindex_superblock_lock.mark_deleted();
        }
    }
    return true;
}
#endif

#if SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::drop_all_sindexes(
        superblock_t *super_block,
        value_sizer_t<void> *sizer,
        value_deleter_t *deleter,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
#else
template <class protocol_t>
void btree_store_t<protocol_t>::drop_all_sindexes(
        write_token_pair_t *token_pair,
        transaction_t *txn,
        superblock_t *super_block,
        value_sizer_t<void> *sizer,
        value_deleter_t *deleter,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
#endif
    assert_thread();

    /* First get the sindex block. */
#if SLICE_ALT
    scoped_ptr_t<alt_buf_lock_t> sindex_block;
    // RSI: Callee doesn't use interruptor.
    acquire_sindex_block_for_write(super_block->expose_buf(),
                                   &sindex_block,
                                   super_block->get_sindex_block_id(), interruptor);
#else
    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_write(token_pair, txn, &sindex_block, super_block->get_sindex_block_id(), interruptor);
#endif

    /* Remove reference in the super block */
    std::map<std::string, secondary_index_t> sindexes;
#if SLICE_ALT
    get_secondary_indexes(sindex_block.get(), &sindexes);
#else
    get_secondary_indexes(txn, sindex_block.get(), &sindexes);
#endif

#if SLICE_ALT
    delete_all_secondary_indexes(sindex_block.get());
#else
    delete_all_secondary_indexes(txn, sindex_block.get());
#endif
    // RSI: Apparently we wanted to release sindex_block so that others may proceed.
#if !SLICE_ALT
    sindex_block->release(); //So others may proceed
#endif


    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        /* Make sure we have a record of the slice. */
        guarantee(std_contains(secondary_index_slices, it->first));
        btree_slice_t *sindex_slice = &(secondary_index_slices.at(it->first));

        {
#if SLICE_ALT
            alt_buf_lock_t sindex_superblock_lock(sindex_block.get(),
                                                  it->second.superblock,
                                                  alt_access_t::write);
#else
            buf_lock_t sindex_superblock_lock(txn, it->second.superblock, rwi_write);
#endif
            real_superblock_t sindex_superblock(&sindex_superblock_lock);

#if SLICE_ALT
            erase_all(sizer, sindex_slice,
                      deleter, &sindex_superblock, interruptor);
#else
            erase_all(sizer, sindex_slice,
                      deleter, txn, &sindex_superblock, interruptor);
#endif
        }

        secondary_index_slices.erase(it->first);

        {
#if SLICE_ALT
            alt_buf_lock_t sindex_superblock_lock(sindex_block.get(),
                                                  it->second.superblock,
                                                  alt_access_t::write);
#else
            buf_lock_t sindex_superblock_lock(txn, it->second.superblock, rwi_write);
#endif
            sindex_superblock_lock.mark_deleted();
        }
    }
#if SLICE_ALT
    sindex_block->reset_buf_lock();
#endif
}

#if SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::get_sindexes(
        superblock_t *super_block,
        std::map<std::string, secondary_index_t> *sindexes_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
#else
template <class protocol_t>
void btree_store_t<protocol_t>::get_sindexes(
        read_token_pair_t *token_pair,
        transaction_t *txn,
        superblock_t *super_block,
        std::map<std::string, secondary_index_t> *sindexes_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
#endif
#if SLICE_ALT
    scoped_ptr_t<alt_buf_lock_t> sindex_block;
    // RSI: We don't actually use interruptor in this called function anymore.
    acquire_sindex_block_for_read(super_block->expose_buf(),
                                  &sindex_block,
                                  super_block->get_sindex_block_id(),
                                  interruptor);
#else
    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_read(token_pair, txn, &sindex_block, super_block->get_sindex_block_id(), interruptor);
#endif

#if SLICE_ALT
    return get_secondary_indexes(sindex_block.get(), sindexes_out);
#else
    return get_secondary_indexes(txn, sindex_block.get(), sindexes_out);
#endif
}

#if SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::get_sindexes(
        alt_buf_lock_t *sindex_block,
        std::map<std::string, secondary_index_t> *sindexes_out)
    THROWS_NOTHING {
    return get_secondary_indexes(sindex_block, sindexes_out);
}
#else
template <class protocol_t>
void btree_store_t<protocol_t>::get_sindexes(
        buf_lock_t *sindex_block,
        transaction_t *txn,
        std::map<std::string, secondary_index_t> *sindexes_out)
    THROWS_NOTHING {
    return get_secondary_indexes(txn, sindex_block, sindexes_out);
}
#endif

#if SLICE_ALT
template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::acquire_sindex_superblock_for_read(
        const std::string &id,
        block_id_t sindex_block_id,
        alt_buf_parent_t parent,
        scoped_ptr_t<real_superblock_t> *sindex_sb_out,
        std::vector<char> *opaque_definition_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, sindex_not_post_constructed_exc_t) {
#else
template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::acquire_sindex_superblock_for_read(
        const std::string &id,
        block_id_t sindex_block_id,
        read_token_pair_t *token_pair,
        transaction_t *txn,
        scoped_ptr_t<real_superblock_t> *sindex_sb_out,
        std::vector<char> *opaque_definition_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, sindex_not_post_constructed_exc_t) {
#endif
    assert_thread();

    /* Acquire the sindex block. */
#if SLICE_ALT
    scoped_ptr_t<alt_buf_lock_t> sindex_block;
#else
    scoped_ptr_t<buf_lock_t> sindex_block;
#endif
#if SLICE_ALT
    // RSI: We don't actually use interruptor in this called function anymore.
    acquire_sindex_block_for_read(parent, &sindex_block,
                                  sindex_block_id, interruptor);
#else
    acquire_sindex_block_for_read(token_pair, txn, &sindex_block, sindex_block_id, interruptor);
#endif

    /* Figure out what the superblock for this index is. */
    secondary_index_t sindex;
#if SLICE_ALT
    if (!::get_secondary_index(sindex_block.get(), id, &sindex)) {
#else
    if (!::get_secondary_index(txn, sindex_block.get(), id, &sindex)) {
#endif
        return false;
    }

    if (opaque_definition_out != NULL) {
        *opaque_definition_out = sindex.opaque_definition;
    }

    if (!sindex.post_construction_complete) {
        throw sindex_not_post_constructed_exc_t(id);
    }

#if SLICE_ALT
    alt_buf_lock_t superblock_lock(sindex_block.get(), sindex.superblock,
                                   alt_access_t::read);
    sindex_block->reset_buf_lock();
#else
    buf_lock_t superblock_lock(txn, sindex.superblock, rwi_read);
#endif
    sindex_sb_out->init(new real_superblock_t(&superblock_lock));
    return true;
}

#if SLICE_ALT
template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::acquire_sindex_superblock_for_write(
        const std::string &id,
        block_id_t sindex_block_id,
        alt_buf_parent_t parent,
        scoped_ptr_t<real_superblock_t> *sindex_sb_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, sindex_not_post_constructed_exc_t) {
#else
template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::acquire_sindex_superblock_for_write(
        const std::string &id,
        block_id_t sindex_block_id,
        write_token_pair_t *token_pair,
        transaction_t *txn,
        scoped_ptr_t<real_superblock_t> *sindex_sb_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, sindex_not_post_constructed_exc_t) {
#endif
    assert_thread();

    /* Get the sindex block. */
#if SLICE_ALT
    scoped_ptr_t<alt_buf_lock_t> sindex_block;
    // RSI: Callee doesn't use interruptor.
    acquire_sindex_block_for_write(parent, &sindex_block,
                                   sindex_block_id, interruptor);
#else
    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_write(token_pair, txn, &sindex_block, sindex_block_id, interruptor);
#endif

    /* Figure out what the superblock for this index is. */
    secondary_index_t sindex;
#if SLICE_ALT
    if (!::get_secondary_index(sindex_block.get(), id, &sindex)) {
#else
    if (!::get_secondary_index(txn, sindex_block.get(), id, &sindex)) {
#endif
        return false;
    }

    if (!sindex.post_construction_complete) {
        throw sindex_not_post_constructed_exc_t(id);
    }


#if SLICE_ALT
    alt_buf_lock_t superblock_lock(sindex_block.get(), sindex.superblock,
                                   alt_access_t::write);
#else
    buf_lock_t superblock_lock(txn, sindex.superblock, rwi_write);
#endif
    sindex_sb_out->init(new real_superblock_t(&superblock_lock));
    return true;
}

#if SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::acquire_all_sindex_superblocks_for_write(
        block_id_t sindex_block_id,
        alt_buf_parent_t parent,
        sindex_access_vector_t *sindex_sbs_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, sindex_not_post_constructed_exc_t) {
#else
template <class protocol_t>
void btree_store_t<protocol_t>::acquire_all_sindex_superblocks_for_write(
        block_id_t sindex_block_id,
        write_token_pair_t *token_pair,
        transaction_t *txn,
        sindex_access_vector_t *sindex_sbs_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, sindex_not_post_constructed_exc_t) {
#endif
    assert_thread();

    /* Get the sindex block. */
#if SLICE_ALT
    scoped_ptr_t<alt_buf_lock_t> sindex_block;
    // RSI: Callee doesn't use interruptor.
    acquire_sindex_block_for_write(parent, &sindex_block,
                                   sindex_block_id, interruptor);
#else
    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_write(token_pair, txn, &sindex_block, sindex_block_id, interruptor);
#endif

#if SLICE_ALT
    acquire_all_sindex_superblocks_for_write(sindex_block.get(), sindex_sbs_out);
#else
    acquire_all_sindex_superblocks_for_write(sindex_block.get(), txn, sindex_sbs_out);
#endif
}

#if SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::acquire_all_sindex_superblocks_for_write(
        alt_buf_lock_t *sindex_block,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
#else
template <class protocol_t>
void btree_store_t<protocol_t>::acquire_all_sindex_superblocks_for_write(
        buf_lock_t *sindex_block,
        transaction_t *txn,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
#endif
#if SLICE_ALT
    acquire_sindex_superblocks_for_write(
            boost::optional<std::set<std::string> >(),
            sindex_block,
            sindex_sbs_out);
#else
    acquire_sindex_superblocks_for_write(
            boost::optional<std::set<std::string> >(),
            sindex_block, txn,
            sindex_sbs_out);
#endif
}

#if SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::acquire_post_constructed_sindex_superblocks_for_write(
        block_id_t sindex_block_id,
        alt_buf_parent_t parent,
        sindex_access_vector_t *sindex_sbs_out,
        signal_t *interruptor)
#else
template <class protocol_t>
void btree_store_t<protocol_t>::acquire_post_constructed_sindex_superblocks_for_write(
        block_id_t sindex_block_id,
        write_token_pair_t *token_pair,
        transaction_t *txn,
        sindex_access_vector_t *sindex_sbs_out,
        signal_t *interruptor)
#endif
    THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

#if SLICE_ALT
    scoped_ptr_t<alt_buf_lock_t> sindex_block;
    // RSI: Callee doesn't use interruptor.
    acquire_sindex_block_for_write(parent, &sindex_block,
                                   sindex_block_id, interruptor);
#else
    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_write(token_pair, txn, &sindex_block, sindex_block_id, interruptor);
#endif

#if SLICE_ALT
    acquire_post_constructed_sindex_superblocks_for_write(
            sindex_block.get(),
            sindex_sbs_out);
#else
    acquire_post_constructed_sindex_superblocks_for_write(
            sindex_block.get(),
            txn,
            sindex_sbs_out);
#endif
}

#if SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::acquire_post_constructed_sindex_superblocks_for_write(
        alt_buf_lock_t *sindex_block,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_NOTHING {
#else
template <class protocol_t>
void btree_store_t<protocol_t>::acquire_post_constructed_sindex_superblocks_for_write(
        buf_lock_t *sindex_block,
        transaction_t *txn,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_NOTHING {
#endif
    assert_thread();
    std::set<std::string> sindexes_to_acquire;
    std::map<std::string, secondary_index_t> sindexes;
#if SLICE_ALT
    ::get_secondary_indexes(sindex_block, &sindexes);
#else
    ::get_secondary_indexes(txn, sindex_block, &sindexes);
#endif

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (it->second.post_construction_complete) {
            sindexes_to_acquire.insert(it->first);
        }
    }

#if SLICE_ALT
    acquire_sindex_superblocks_for_write(
            sindexes_to_acquire, sindex_block,
            sindex_sbs_out);
#else
    acquire_sindex_superblocks_for_write(
            sindexes_to_acquire, sindex_block,
            txn, sindex_sbs_out);
#endif
}

#if SLICE_ALT
template <class protocol_t>
bool btree_store_t<protocol_t>::acquire_sindex_superblocks_for_write(
            boost::optional<std::set<std::string> > sindexes_to_acquire, //none means acquire all sindexes
            alt_buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
#else
template <class protocol_t>
bool btree_store_t<protocol_t>::acquire_sindex_superblocks_for_write(
            boost::optional<std::set<std::string> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            transaction_t *txn,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
#endif
    assert_thread();

    std::map<std::string, secondary_index_t> sindexes;
#if SLICE_ALT
    ::get_secondary_indexes(sindex_block, &sindexes);
#else
    ::get_secondary_indexes(txn, sindex_block, &sindexes);
#endif

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (sindexes_to_acquire && !std_contains(*sindexes_to_acquire, it->first)) {
            continue;
        }

        /* Getting the slice and asserting we're on the right thread. */
        btree_slice_t *sindex_slice = &(secondary_index_slices.at(it->first));
        sindex_slice->assert_thread();

        /* Notice this looks like a bug but isn't. This buf_lock_t will indeed
         * get destructed at the end of this function but passing it to the
         * real_superblock_t constructor below swaps out its acual lock so it's
         * just default constructed lock when it gets destructed. */
        // RSI: Yawn, just use std::move.
#if SLICE_ALT
        alt_buf_lock_t superblock_lock(sindex_block, it->second.superblock,
                                       alt_access_t::write);
#else
        buf_lock_t superblock_lock(txn, it->second.superblock, rwi_write);
#endif

        sindex_sbs_out->push_back(new
                sindex_access_t(get_sindex_slice(it->first), it->second, new
                    real_superblock_t(&superblock_lock)));
    }

    //return's true if we got all of the sindexes requested.
    return sindex_sbs_out->size() == sindexes_to_acquire->size();
}

#if SLICE_ALT
template <class protocol_t>
bool btree_store_t<protocol_t>::acquire_sindex_superblocks_for_write(
            boost::optional<std::set<uuid_u> > sindexes_to_acquire, //none means acquire all sindexes
            alt_buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
#else
template <class protocol_t>
bool btree_store_t<protocol_t>::acquire_sindex_superblocks_for_write(
            boost::optional<std::set<uuid_u> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            transaction_t *txn,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
#endif
    assert_thread();

    std::map<std::string, secondary_index_t> sindexes;
#if SLICE_ALT
    ::get_secondary_indexes(sindex_block, &sindexes);
#else
    ::get_secondary_indexes(txn, sindex_block, &sindexes);
#endif

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (sindexes_to_acquire && !std_contains(*sindexes_to_acquire, it->second.id)) {
            continue;
        }

        /* Getting the slice and asserting we're on the right thread. */
        btree_slice_t *sindex_slice = &(secondary_index_slices.at(it->first));
        sindex_slice->assert_thread();

        /* Notice this looks like a bug but isn't. This buf_lock_t will indeed
         * get destructed at the end of this function but passing it to the
         * real_superblock_t constructor below swaps out its acual lock so it's
         * just default constructed lock when it gets destructed. */
        // RSI: copy/pasted comment?  Use std::move thx.
#if SLICE_ALT
        alt_buf_lock_t superblock_lock(sindex_block,
                                       it->second.superblock,
                                       alt_access_t::write);
#else
        buf_lock_t superblock_lock(txn, it->second.superblock, rwi_write);
#endif

        sindex_sbs_out->push_back(new
                sindex_access_t(get_sindex_slice(it->first), it->second, new
                    real_superblock_t(&superblock_lock)));
    }

    //return's true if we got all of the sindexes requested.
    return sindex_sbs_out->size() == sindexes_to_acquire->size();
}

template <class protocol_t>
void btree_store_t<protocol_t>::check_and_update_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
        const metainfo_t &new_metainfo,
#if !SLICE_ALT
        transaction_t *txn,
#endif
        real_superblock_t *superblock) const
        THROWS_NOTHING {
    assert_thread();
#if SLICE_ALT
    metainfo_t old_metainfo = check_metainfo(DEBUG_ONLY(metainfo_checker, ) superblock);
    update_metainfo(old_metainfo, new_metainfo, superblock);
#else
    metainfo_t old_metainfo = check_metainfo(DEBUG_ONLY(metainfo_checker, ) txn, superblock);
    update_metainfo(old_metainfo, new_metainfo, txn, superblock);
#endif
}

template <class protocol_t>
typename btree_store_t<protocol_t>::metainfo_t
btree_store_t<protocol_t>::check_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
#if !SLICE_ALT
        transaction_t *txn,
#endif
        real_superblock_t *superblock) const
        THROWS_NOTHING {
    assert_thread();
    region_map_t<protocol_t, binary_blob_t> old_metainfo;
#if SLICE_ALT
    get_metainfo_internal(superblock->get(), &old_metainfo);
#else
    get_metainfo_internal(txn, superblock->get(), &old_metainfo);
#endif
#ifndef NDEBUG
    metainfo_checker.check_metainfo(old_metainfo.mask(metainfo_checker.get_domain()));
#endif
    return old_metainfo;
}

#if SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::update_metainfo(const metainfo_t &old_metainfo,
                                                const metainfo_t &new_metainfo,
                                                real_superblock_t *superblock)
    const THROWS_NOTHING {
#else
template <class protocol_t>
void btree_store_t<protocol_t>::update_metainfo(const metainfo_t &old_metainfo, const metainfo_t &new_metainfo, transaction_t *txn, real_superblock_t *superblock) const THROWS_NOTHING {
#endif
    assert_thread();
    region_map_t<protocol_t, binary_blob_t> updated_metadata = old_metainfo;
    updated_metadata.update(new_metainfo);

    rassert(updated_metadata.get_domain() == protocol_t::region_t::universe());

#if SLICE_ALT
    alt_buf_lock_t *sb_buf = superblock->get();
    clear_superblock_metainfo(sb_buf);
#else
    buf_lock_t *sb_buf = superblock->get();
    clear_superblock_metainfo(txn, sb_buf);
#endif

    for (typename region_map_t<protocol_t, binary_blob_t>::const_iterator i = updated_metadata.begin(); i != updated_metadata.end(); ++i) {
        vector_stream_t key;
        write_message_t msg;
        msg << i->first;
        DEBUG_VAR int res = send_write_message(&key, &msg);
        rassert(!res);

        std::vector<char> value(static_cast<const char*>(i->second.data()),
                                static_cast<const char*>(i->second.data()) + i->second.size());

#if SLICE_ALT
        set_superblock_metainfo(sb_buf, key.vector(), value); // FIXME: this is not efficient either, see how value is created
#else
        set_superblock_metainfo(txn, sb_buf, key.vector(), value); // FIXME: this is not efficient either, see how value is created
#endif
    }
}

template <class protocol_t>
void btree_store_t<protocol_t>::do_get_metainfo(UNUSED order_token_t order_token,  // TODO
                                                object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
                                                signal_t *interruptor,
                                                metainfo_t *out) THROWS_ONLY(interrupted_exc_t) {
    debugf_t eex("%p do_get_metainfo", this);

    assert_thread();
#if SLICE_ALT
    scoped_ptr_t<alt_txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_read(token,
                                &txn, &superblock,
                                interruptor,
                                false /* RSI: christ */);

    get_metainfo_internal(superblock->get(), out);
#else
    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_read(rwi_read, token, &txn, &superblock, interruptor, false);

    get_metainfo_internal(txn.get(), superblock->get(), out);
#endif
}

#if SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::
get_metainfo_internal(alt_buf_lock_t *sb_buf,
                      region_map_t<protocol_t, binary_blob_t> *out)
    const THROWS_NOTHING {
#else
template <class protocol_t>
void btree_store_t<protocol_t>::get_metainfo_internal(transaction_t *txn, buf_lock_t *sb_buf, region_map_t<protocol_t, binary_blob_t> *out) const THROWS_NOTHING {
#endif
    assert_thread();
    std::vector<std::pair<std::vector<char>, std::vector<char> > > kv_pairs;
#if SLICE_ALT
    // TODO: this is inefficient, cut out the middleman (vector)
    get_superblock_metainfo(sb_buf, &kv_pairs);
#else
    get_superblock_metainfo(txn, sb_buf, &kv_pairs);   // FIXME: this is inefficient, cut out the middleman (vector)
#endif

    std::vector<std::pair<typename protocol_t::region_t, binary_blob_t> > result;
    for (std::vector<std::pair<std::vector<char>, std::vector<char> > >::iterator i = kv_pairs.begin(); i != kv_pairs.end(); ++i) {
        const std::vector<char> &value = i->second;

        typename protocol_t::region_t region;
        {
            vector_read_stream_t key(&i->first);
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
    debugf_t eex("%p set_metainfo", this);

    assert_thread();

#if SLICE_ALT
    scoped_ptr_t<alt_txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_write(alt_access_t::write,
                                 repli_timestamp_t::invalid,
                                 1,
                                 write_durability_t::HARD,
                                 token,
                                 &txn,
                                 &superblock,
                                 interruptor);
#else
    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_write(rwi_write, rwi_write,
                                 repli_timestamp_t::invalid,
                                 1,
                                 write_durability_t::HARD,
                                 token,
                                 &txn,
                                 &superblock,
                                 interruptor);
#endif

    region_map_t<protocol_t, binary_blob_t> old_metainfo;
#if SLICE_ALT
    get_metainfo_internal(superblock->get(), &old_metainfo);
    update_metainfo(old_metainfo, new_metainfo, superblock.get());
#else
    get_metainfo_internal(txn.get(), superblock->get(), &old_metainfo);
    update_metainfo(old_metainfo, new_metainfo, txn.get(), superblock.get());
#endif
}

#if SLICE_ALT
template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_read(
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
        scoped_ptr_t<alt_txn_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor,
        bool use_snapshot)
        THROWS_ONLY(interrupted_exc_t) {
#else
template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_read(
        access_t access,
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
        scoped_ptr_t<transaction_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor,
        bool use_snapshot)
        THROWS_ONLY(interrupted_exc_t) {
#endif

    assert_thread();
    btree->assert_thread();

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(token);
    wait_interruptible(token->get(), interruptor);

    order_token_t order_token = order_source.check_in("btree_store_t<" + protocol_t::protocol_name + ">::acquire_superblock_for_read").with_read_mode();
    order_token = btree->pre_begin_txn_checkpoint_.check_through(order_token);

    cache_snapshotted_t cache_snapshotted =
        use_snapshot ? CACHE_SNAPSHOTTED_YES : CACHE_SNAPSHOTTED_NO;
#if SLICE_ALT
    get_btree_superblock_and_txn_for_reading(
        btree.get(), order_token, cache_snapshotted, sb_out, txn_out);
#else
    get_btree_superblock_and_txn_for_reading(
        btree.get(), access, order_token, cache_snapshotted, sb_out, txn_out);
#endif
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_backfill(
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
#if SLICE_ALT
        scoped_ptr_t<alt_txn_t> *txn_out,
#else
        scoped_ptr_t<transaction_t> *txn_out,
#endif
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    assert_thread();
    btree->assert_thread();

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(token);
    wait_interruptible(token->get(), interruptor);

    order_token_t order_token = order_source.check_in("btree_store_t<" + protocol_t::protocol_name + ">::acquire_superblock_for_backfill");
    order_token = btree->pre_begin_txn_checkpoint_.check_through(order_token);

    get_btree_superblock_and_txn_for_backfilling(btree.get(), order_token, sb_out, txn_out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_write(
        repli_timestamp_t timestamp,
        int expected_change_count,
        const write_durability_t durability,
        write_token_pair_t *token_pair,
#if SLICE_ALT
        scoped_ptr_t<alt_txn_t> *txn_out,
#else
        scoped_ptr_t<transaction_t> *txn_out,
#endif
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

#if SLICE_ALT
    acquire_superblock_for_write(alt_access_t::write, timestamp,
                                 expected_change_count, durability,
                                 token_pair, txn_out, sb_out, interruptor);
#else
    acquire_superblock_for_write(rwi_write, rwi_write, timestamp, expected_change_count, durability,
            token_pair, txn_out, sb_out, interruptor);
#endif
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_write(
#if SLICE_ALT
        alt_access_t superblock_access,
#else
        access_t txn_access,
        access_t superblock_access,
#endif
        repli_timestamp_t timestamp,
        int expected_change_count,
        const write_durability_t durability,
        write_token_pair_t *token_pair,
#if SLICE_ALT
        scoped_ptr_t<alt_txn_t> *txn_out,
#else
        scoped_ptr_t<transaction_t> *txn_out,
#endif
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

#if SLICE_ALT
    acquire_superblock_for_write(superblock_access, timestamp,
                                 expected_change_count, durability,
                                 &token_pair->main_write_token, txn_out, sb_out,
                                 interruptor);
#else
    acquire_superblock_for_write(txn_access, superblock_access, timestamp, expected_change_count, durability,
            &token_pair->main_write_token, txn_out, sb_out, interruptor);
#endif
    // RSI: Do whatever (*txn_out)->set_token_pair does.
#if !SLICE_ALT
    (*txn_out)->set_token_pair(token_pair);
#endif
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_write(
#if SLICE_ALT
        alt_access_t superblock_access,
#else
        access_t txn_access,
        access_t superblock_access,
#endif
        repli_timestamp_t timestamp,
        int expected_change_count,
        write_durability_t durability,
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
#if SLICE_ALT
        scoped_ptr_t<alt_txn_t> *txn_out,
#else
        scoped_ptr_t<transaction_t> *txn_out,
#endif
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    assert_thread();
    btree->assert_thread();

    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(token);
    wait_interruptible(token->get(), interruptor);

    order_token_t order_token = order_source.check_in("btree_store_t<" + protocol_t::protocol_name + ">::acquire_superblock_for_write");
    order_token = btree->pre_begin_txn_checkpoint_.check_through(order_token);

#if SLICE_ALT
    get_btree_superblock_and_txn(btree.get(), superblock_access,
                                 expected_change_count, timestamp, order_token,
                                 durability, sb_out, txn_out);
#else
    get_btree_superblock_and_txn(btree.get(), txn_access, superblock_access, expected_change_count, timestamp, order_token, durability, sb_out, txn_out);
#endif
}

/* store_view_t interface */
template <class protocol_t>
void btree_store_t<protocol_t>::new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token_out) {
    debugf_t eex("%p new_read_token", this);
    assert_thread();
    fifo_enforcer_read_token_t token = main_token_source.enter_read();
    token_out->create(&main_token_sink, token);
}

template <class protocol_t>
void btree_store_t<protocol_t>::new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token_out) {
    debugf_t eex("%p new_write_token", this);
    assert_thread();
    fifo_enforcer_write_token_t token = main_token_source.enter_write();
    token_out->create(&main_token_sink, token);
}

template <class protocol_t>
void btree_store_t<protocol_t>::new_read_token_pair(read_token_pair_t *token_pair_out) {
    assert_thread();
    debugf_t eex("%p new_read_token_pair", this);
#if !SLICE_ALT
    fifo_enforcer_read_token_t token = sindex_token_source.enter_read();
    token_pair_out->sindex_read_token.create(&sindex_token_sink, token);
#endif

    new_read_token(&(token_pair_out->main_read_token));
}

template <class protocol_t>
void btree_store_t<protocol_t>::new_write_token_pair(write_token_pair_t *token_pair_out) {
    debugf_t eex("%p new_write_token_pair", this);
    assert_thread();
#if !SLICE_ALT
    fifo_enforcer_write_token_t token = sindex_token_source.enter_write();
    token_pair_out->sindex_write_token.create(&sindex_token_sink, token);
#endif

    new_write_token(&(token_pair_out->main_write_token));
}

#include "memcached/protocol.hpp"
template class btree_store_t<memcached_protocol_t>;

#include "rdb_protocol/protocol.hpp"
template class btree_store_t<rdb_protocol_t>;
