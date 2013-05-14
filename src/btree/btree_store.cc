// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "btree/btree_store.hpp"

#include "btree/operations.hpp"
#include "btree/secondary_operations.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/vector_stream.hpp"
#include "serializer/config.hpp"
#include "stl_utils.hpp"

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
    if (create) {
        cache_t::create(serializer);
    }

    // TODO: Don't specify cache dynamic config here.
    cache_dynamic_config.max_size = cache_target;
    cache_dynamic_config.max_dirty_size = cache_target / 2;
    cache.init(new cache_t(serializer, cache_dynamic_config, &perfmon_collection));

    if (create) {
        vector_stream_t key;
        write_message_t msg;
        typename protocol_t::region_t kr = protocol_t::region_t::universe();   // `operator<<` needs a non-const reference  // TODO <- what
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

        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        acquire_superblock_for_read(rwi_read, &token_pair.main_read_token, &txn,
                                    &superblock, &dummy_interruptor, false);

        scoped_ptr_t<buf_lock_t> sindex_block;
        acquire_sindex_block_for_read(&token_pair, txn.get(), &sindex_block,
                                      superblock->get_sindex_block_id(),
                                      &dummy_interruptor);

        std::map<std::string, secondary_index_t> sindexes;
        get_secondary_indexes(txn.get(), sindex_block.get(), &sindexes);

        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            secondary_index_slices.insert(it->first,
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
    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;

    acquire_superblock_for_read(rwi_read, &token_pair->main_read_token, &txn, &superblock, interruptor,
                                read.use_snapshot());

    check_metainfo(DEBUG_ONLY(metainfo_checker, ) txn.get(), superblock.get());

    // Ugly hack
    scoped_ptr_t<superblock_t> superblock2;
    superblock2.init(superblock.release());

    protocol_read(read, response, btree.get(), txn.get(), superblock2.get(), token_pair, interruptor);
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

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> real_superblock;
    const int expected_change_count = 2; // FIXME: this is incorrect, but will do for now
    acquire_superblock_for_write(rwi_write, timestamp.to_repli_timestamp(), expected_change_count, durability, token_pair, &txn, &real_superblock, interruptor);

    check_and_update_metainfo(DEBUG_ONLY(metainfo_checker, ) new_metainfo, txn.get(), real_superblock.get());
    scoped_ptr_t<superblock_t> superblock(real_superblock.release());
    protocol_write(write, response, timestamp, btree.get(), txn.get(), &superblock, token_pair, interruptor);
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

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_backfill(&token_pair->main_read_token, &txn, &superblock, interruptor);

    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_read(token_pair, txn.get(), &sindex_block, superblock->get_sindex_block_id(), interruptor);

    region_map_t<protocol_t, binary_blob_t> unmasked_metainfo;
    get_metainfo_internal(txn.get(), superblock->get(), &unmasked_metainfo);
    region_map_t<protocol_t, binary_blob_t> metainfo = unmasked_metainfo.mask(start_point.get_domain());
    if (send_backfill_cb->should_backfill(metainfo)) {
        protocol_send_backfill(start_point, send_backfill_cb, superblock.get(), sindex_block.get(), btree.get(), txn.get(), progress, interruptor);
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

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    const int expected_change_count = 1; // FIXME: this is probably not correct

    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t
        token_destroyer(&token_pair->sindex_write_token);

    // We don't want hard durability, this is a backfill chunk, and nobody
    // wants chunk-by-chunk acks.
    acquire_superblock_for_write(rwi_write,
                                 chunk.get_btree_repli_timestamp(),
                                 expected_change_count,
                                 WRITE_DURABILITY_SOFT,
                                 token_pair,
                                 &txn,
                                 &superblock,
                                 interruptor);

    protocol_receive_backfill(btree.get(),
                              txn.get(),
                              superblock.get(),
                              token_pair,
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

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;

    // We're passing 2 for the expected_change_count based on the
    // reasoning that we're probably going to touch a leaf-node-sized
    // range of keys and that it won't be aligned right on a leaf node
    // boundary.
    // TOnDO that's not reasonable; reset_data() is sometimes used to wipe out
    // entire databases.
    const int expected_change_count = 2;
    acquire_superblock_for_write(rwi_write,
                                 repli_timestamp_t::invalid,
                                 expected_change_count,
                                 durability,
                                 token_pair,
                                 &txn,
                                 &superblock,
                                 interruptor);

    region_map_t<protocol_t, binary_blob_t> old_metainfo;
    get_metainfo_internal(txn.get(), superblock->get(), &old_metainfo);
    update_metainfo(old_metainfo, new_metainfo, txn.get(), superblock.get());

    protocol_reset_data(subregion,
                        btree.get(),
                        txn.get(),
                        superblock.get(),
                        token_pair,
                        interruptor);
}

template <class protocol_t>
void btree_store_t<protocol_t>::lock_sindex_queue(buf_lock_t *sindex_block, mutex_t::acq_t *acq) {
    assert_thread();
    guarantee(sindex_block->is_acquired());
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
void btree_store_t<protocol_t>::acquire_sindex_block_for_read(
        read_token_pair_t *token_pair,
        transaction_t *txn,
        scoped_ptr_t<buf_lock_t> *sindex_block_out,
        block_id_t sindex_block_id,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
    /* First wait for our turn. */
    wait_interruptible(token_pair->sindex_read_token.get(), interruptor);

    /* Make sure others will be allowed to proceed after this function is
     * completes. */
    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(&token_pair->sindex_read_token);

    /* Finally acquire the block. */
    sindex_block_out->init(new buf_lock_t(txn, sindex_block_id, rwi_read));
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_sindex_block_for_write(
        write_token_pair_t *token_pair,
        transaction_t *txn,
        scoped_ptr_t<buf_lock_t> *sindex_block_out,
        block_id_t sindex_block_id,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {

    /* First wait for our turn. */
    wait_interruptible(token_pair->sindex_write_token.get(), interruptor);

    /* Make sure others will be allowed to proceed after this function is
     * completes. */
    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(&token_pair->sindex_write_token);

    /* Finally acquire the block. */
    sindex_block_out->init(new buf_lock_t(txn, sindex_block_id, rwi_write));
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

template <class protocol_t>
bool btree_store_t<protocol_t>::mark_index_up_to_date(
    const std::string &id,
    transaction_t *txn,
    buf_lock_t *sindex_block)
THROWS_NOTHING {
    secondary_index_t sindex;
    bool found = ::get_secondary_index(txn, sindex_block, id, &sindex);

    if (found) {
        sindex.post_construction_complete = true;

        ::set_secondary_index(txn, sindex_block, id, sindex);
    }

    return found;
}

template <class protocol_t>
bool btree_store_t<protocol_t>::mark_index_up_to_date(
    uuid_u id,
    transaction_t *txn,
    buf_lock_t *sindex_block)
THROWS_NOTHING {
    secondary_index_t sindex;
    bool found = ::get_secondary_index(txn, sindex_block, id, &sindex);

    if (found) {
        sindex.post_construction_complete = true;

        ::set_secondary_index(txn, sindex_block, id, sindex);
    }

    return found;
}

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

template <class protocol_t>
void btree_store_t<protocol_t>::drop_all_sindexes(
        write_token_pair_t *token_pair,
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
    std::map<std::string, secondary_index_t> sindexes;
    get_secondary_indexes(txn, sindex_block.get(), &sindexes);

    delete_all_secondary_indexes(txn, sindex_block.get());
    sindex_block->release(); //So others may proceed


    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        /* Make sure we have a record of the slice. */
        guarantee(std_contains(secondary_index_slices, it->first));
        btree_slice_t *sindex_slice = &(secondary_index_slices.at(it->first));

        {
            buf_lock_t sindex_superblock_lock(txn, it->second.superblock, rwi_write);
            real_superblock_t sindex_superblock(&sindex_superblock_lock);

            erase_all(sizer, sindex_slice,
                      deleter, txn, &sindex_superblock, interruptor);
        }

        secondary_index_slices.erase(it->first);

        {
            buf_lock_t sindex_superblock_lock(txn, it->second.superblock, rwi_write);
            sindex_superblock_lock.mark_deleted();
        }
    }
}

template <class protocol_t>
void btree_store_t<protocol_t>::get_sindexes(
        read_token_pair_t *token_pair,
        transaction_t *txn,
        superblock_t *super_block,
        std::map<std::string, secondary_index_t> *sindexes_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_read(token_pair, txn, &sindex_block, super_block->get_sindex_block_id(), interruptor);

    return get_secondary_indexes(txn, sindex_block.get(), sindexes_out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::get_sindexes(
        buf_lock_t *sindex_block,
        transaction_t *txn,
        std::map<std::string, secondary_index_t> *sindexes_out)
    THROWS_NOTHING {
    return get_secondary_indexes(txn, sindex_block, sindexes_out);
}

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
    assert_thread();

    /* Acquire the sindex block. */
    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_read(token_pair, txn, &sindex_block, sindex_block_id, interruptor);

    /* Figure out what the superblock for this index is. */
    secondary_index_t sindex;
    if (!::get_secondary_index(txn, sindex_block.get(), id, &sindex)) {
        return false;
    }

    if (opaque_definition_out != NULL) {
        *opaque_definition_out = sindex.opaque_definition;
    }

    if (!sindex.post_construction_complete) {
        throw sindex_not_post_constructed_exc_t(id);
    }

    buf_lock_t superblock_lock(txn, sindex.superblock, rwi_read);
    sindex_sb_out->init(new real_superblock_t(&superblock_lock));
    return true;
}

template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::acquire_sindex_superblock_for_write(
        const std::string &id,
        block_id_t sindex_block_id,
        write_token_pair_t *token_pair,
        transaction_t *txn,
        scoped_ptr_t<real_superblock_t> *sindex_sb_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, sindex_not_post_constructed_exc_t) {
    assert_thread();

    /* Get the sindex block. */
    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_write(token_pair, txn, &sindex_block, sindex_block_id, interruptor);

    /* Figure out what the superblock for this index is. */
    secondary_index_t sindex;
    if (!::get_secondary_index(txn, sindex_block.get(), id, &sindex)) {
        return false;
    }

    if (!sindex.post_construction_complete) {
        throw sindex_not_post_constructed_exc_t(id);
    }


    buf_lock_t superblock_lock(txn, sindex.superblock, rwi_write);
    sindex_sb_out->init(new real_superblock_t(&superblock_lock));
    return true;
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_all_sindex_superblocks_for_write(
        block_id_t sindex_block_id,
        write_token_pair_t *token_pair,
        transaction_t *txn,
        sindex_access_vector_t *sindex_sbs_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, sindex_not_post_constructed_exc_t) {
    assert_thread();

    /* Get the sindex block. */
    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_write(token_pair, txn, &sindex_block, sindex_block_id, interruptor);

    acquire_all_sindex_superblocks_for_write(sindex_block.get(), txn, sindex_sbs_out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_all_sindex_superblocks_for_write(
        buf_lock_t *sindex_block,
        transaction_t *txn,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
    acquire_sindex_superblocks_for_write(
            boost::optional<std::set<std::string> >(),
            sindex_block, txn,
            sindex_sbs_out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::aquire_post_constructed_sindex_superblocks_for_write(
        block_id_t sindex_block_id,
        write_token_pair_t *token_pair,
        transaction_t *txn,
        sindex_access_vector_t *sindex_sbs_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_write(token_pair, txn, &sindex_block, sindex_block_id, interruptor);

    aquire_post_constructed_sindex_superblocks_for_write(
            sindex_block.get(),
            txn,
            sindex_sbs_out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::aquire_post_constructed_sindex_superblocks_for_write(
        buf_lock_t *sindex_block,
        transaction_t *txn,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_NOTHING {
    assert_thread();
    std::set<std::string> sindexes_to_acquire;
    std::map<std::string, secondary_index_t> sindexes;
    ::get_secondary_indexes(txn, sindex_block, &sindexes);

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (it->second.post_construction_complete) {
            sindexes_to_acquire.insert(it->first);
        }
    }

    acquire_sindex_superblocks_for_write(
            sindexes_to_acquire, sindex_block,
            txn, sindex_sbs_out);
}

template <class protocol_t>
bool btree_store_t<protocol_t>::acquire_sindex_superblocks_for_write(
            boost::optional<std::set<std::string> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            transaction_t *txn,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
    assert_thread();

    std::map<std::string, secondary_index_t> sindexes;
    ::get_secondary_indexes(txn, sindex_block, &sindexes);

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
        buf_lock_t superblock_lock(txn, it->second.superblock, rwi_write);

        sindex_sbs_out->push_back(new
                sindex_access_t(get_sindex_slice(it->first), it->second, new
                    real_superblock_t(&superblock_lock)));
    }
    
    //return's true if we got all of the sindexes requested.
    return sindex_sbs_out->size() == sindexes_to_acquire->size();
}

template <class protocol_t>
bool btree_store_t<protocol_t>::acquire_sindex_superblocks_for_write(
            boost::optional<std::set<uuid_u> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            transaction_t *txn,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
    assert_thread();

    std::map<std::string, secondary_index_t> sindexes;
    ::get_secondary_indexes(txn, sindex_block, &sindexes);

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
        buf_lock_t superblock_lock(txn, it->second.superblock, rwi_write);

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
        transaction_t *txn,
        real_superblock_t *superblock) const
        THROWS_NOTHING {
    assert_thread();
    metainfo_t old_metainfo = check_metainfo(DEBUG_ONLY(metainfo_checker, ) txn, superblock);
    update_metainfo(old_metainfo, new_metainfo, txn, superblock);
}

template <class protocol_t>
typename btree_store_t<protocol_t>::metainfo_t btree_store_t<protocol_t>::check_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
        transaction_t *txn,
        real_superblock_t *superblock) const
        THROWS_NOTHING {
    assert_thread();
    region_map_t<protocol_t, binary_blob_t> old_metainfo;
    get_metainfo_internal(txn, superblock->get(), &old_metainfo);
#ifndef NDEBUG
    metainfo_checker.check_metainfo(old_metainfo.mask(metainfo_checker.get_domain()));
#endif
    return old_metainfo;
}

template <class protocol_t>
void btree_store_t<protocol_t>::update_metainfo(const metainfo_t &old_metainfo, const metainfo_t &new_metainfo, transaction_t *txn, real_superblock_t *superblock) const THROWS_NOTHING {
    assert_thread();
    region_map_t<protocol_t, binary_blob_t> updated_metadata = old_metainfo;
    updated_metadata.update(new_metainfo);

    rassert(updated_metadata.get_domain() == protocol_t::region_t::universe());

    buf_lock_t* sb_buf = superblock->get();
    clear_superblock_metainfo(txn, sb_buf);

    for (typename region_map_t<protocol_t, binary_blob_t>::const_iterator i = updated_metadata.begin(); i != updated_metadata.end(); ++i) {
        vector_stream_t key;
        write_message_t msg;
        msg << i->first;
        DEBUG_VAR int res = send_write_message(&key, &msg);
        rassert(!res);

        std::vector<char> value(static_cast<const char*>(i->second.data()),
                                static_cast<const char*>(i->second.data()) + i->second.size());

        set_superblock_metainfo(txn, sb_buf, key.vector(), value); // FIXME: this is not efficient either, see how value is created
    }
}

template <class protocol_t>
void btree_store_t<protocol_t>::do_get_metainfo(UNUSED order_token_t order_token,  // TODO
                                                object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
                                                signal_t *interruptor,
                                                metainfo_t *out) THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_read(rwi_read, token, &txn, &superblock, interruptor, false);

    get_metainfo_internal(txn.get(), superblock->get(), out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::get_metainfo_internal(transaction_t *txn, buf_lock_t *sb_buf, region_map_t<protocol_t, binary_blob_t> *out) const THROWS_NOTHING {
    assert_thread();
    std::vector<std::pair<std::vector<char>, std::vector<char> > > kv_pairs;
    get_superblock_metainfo(txn, sb_buf, &kv_pairs);   // FIXME: this is inefficient, cut out the middleman (vector)

    std::vector<std::pair<typename protocol_t::region_t, binary_blob_t> > result;
    for (std::vector<std::pair<std::vector<char>, std::vector<char> > >::iterator i = kv_pairs.begin(); i != kv_pairs.end(); ++i) {
        const std::vector<char> &value = i->second;

        typename protocol_t::region_t region;
        {
            vector_read_stream_t key(&i->first);
            DEBUG_VAR archive_result_t res = deserialize(&key, &region);
            rassert(!res, "res = %d", res);
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

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_write(rwi_write,
                                 repli_timestamp_t::invalid,
                                 1,
                                 WRITE_DURABILITY_HARD,
                                 token,
                                 &txn,
                                 &superblock,
                                 interruptor);

    region_map_t<protocol_t, binary_blob_t> old_metainfo;
    get_metainfo_internal(txn.get(), superblock->get(), &old_metainfo);
    update_metainfo(old_metainfo, new_metainfo, txn.get(), superblock.get());
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_read(
        access_t access,
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
        scoped_ptr_t<transaction_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor,
        bool use_snapshot)
        THROWS_ONLY(interrupted_exc_t) {

    assert_thread();
    btree->assert_thread();

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(token);
    wait_interruptible(token->get(), interruptor);

    order_token_t order_token = order_source.check_in("btree_store_t<" + protocol_t::protocol_name + ">::acquire_superblock_for_read").with_read_mode();
    order_token = btree->pre_begin_txn_checkpoint_.check_through(order_token);

    cache_snapshotted_t cache_snapshotted =
        use_snapshot ? CACHE_SNAPSHOTTED_YES : CACHE_SNAPSHOTTED_NO;
    get_btree_superblock_and_txn_for_reading(
        btree.get(), access, order_token, cache_snapshotted, sb_out, txn_out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_backfill(
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
        scoped_ptr_t<transaction_t> *txn_out,
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
        access_t access,
        repli_timestamp_t timestamp,
        int expected_change_count,
        const write_durability_t durability,
        write_token_pair_t *token_pair,
        scoped_ptr_t<transaction_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    acquire_superblock_for_write(access, timestamp, expected_change_count, durability,
            &token_pair->main_write_token, txn_out, sb_out, interruptor);
    (*txn_out)->set_token_pair(token_pair);
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_superblock_for_write(
        access_t access,
        repli_timestamp_t timestamp,
        int expected_change_count,
        write_durability_t durability,
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
        scoped_ptr_t<transaction_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    assert_thread();
    btree->assert_thread();

    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(token);
    wait_interruptible(token->get(), interruptor);

    order_token_t order_token = order_source.check_in("btree_store_t<" + protocol_t::protocol_name + ">::acquire_superblock_for_write");
    order_token = btree->pre_begin_txn_checkpoint_.check_through(order_token);

    get_btree_superblock_and_txn(btree.get(), access, expected_change_count, timestamp, order_token, durability, sb_out, txn_out);
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

template <class protocol_t>
void btree_store_t<protocol_t>::new_read_token_pair(read_token_pair_t *token_pair_out) {
    assert_thread();
    fifo_enforcer_read_token_t token = sindex_token_source.enter_read();
    token_pair_out->sindex_read_token.create(&sindex_token_sink, token);

    new_read_token(&(token_pair_out->main_read_token));
}

template <class protocol_t>
void btree_store_t<protocol_t>::new_write_token_pair(write_token_pair_t *token_pair_out) {
    assert_thread();
    fifo_enforcer_write_token_t token = sindex_token_source.enter_write();
    token_pair_out->sindex_write_token.create(&sindex_token_sink, token);

    new_write_token(&(token_pair_out->main_write_token));
}

#include "memcached/protocol.hpp"
template class btree_store_t<memcached_protocol_t>;

#include "rdb_protocol/protocol.hpp"
template class btree_store_t<rdb_protocol_t>;
