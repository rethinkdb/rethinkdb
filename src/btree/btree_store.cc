// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/btree_store.hpp"

#include "btree/operations.hpp"
#include "btree/secondary_operations.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/cache_balancer.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/disk_backed_queue.hpp"
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
                                         alt_cache_balancer_t *balancer,
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
    {
        alt_cache_config_t config;
        // TODO: don't allow NULL balancers
        if (balancer != NULL) {
            config.page_config.memory_limit = balancer->get_base_mem_per_store();
        } else {
            // TODO: this is really small
            config.page_config.memory_limit = BASE_CACHE_SIZE;
        }
        cache.init(new cache_t(serializer, balancer, config, &perfmon_collection));
        general_cache_conn.init(new cache_conn_t(cache.get()));
    }

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
    const int expected_change_count = 1; // FIXME: this is probably not correct

    // We don't want hard durability, this is a backfill chunk, and nobody
    // wants chunk-by-chunk acks.
    acquire_superblock_for_write(chunk.get_btree_repli_timestamp(),
                                 expected_change_count,
                                 write_durability_t::SOFT,
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
void btree_store_t<protocol_t>::lock_sindex_queue(buf_lock_t *sindex_block,
                                                  mutex_t::acq_t *acq) {
    assert_thread();
    // KSI: Prove that we can remove this "lock_sindex_queue" stuff
    // (probably).

    // SRH: WTF should we do here?  Why is there a mutex?
    // JD: There's a mutex to protect the sindex queue which is an in memory
    // structure.

    // SRH: Do we really need to wait for write acquisition?
    // JD: I think so, the whole idea is to enforce that you get a consistent
    // view. In which everything before is in the main btree and everything
    // after you is in the queue. If we don't have write access when we add
    // ourselves to the queue

    // SRH: Should we be able to "get in line" for the mutex and release the sindex
    // block or something?
    // JD: That could be a good optimization but I don't think there's any reason
    // we need to do it as part of the new cache. There's also an argument to
    // be made that this mutex could just go away and having write access to
    // the sindex block could fill the same role.
    guarantee(!sindex_block->empty());
    sindex_block->write_acq_signal()->wait();
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
        value_sizer_t<void> *sizer,
        value_deleter_t *deleter, signal_t *interruptor) {
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

template <class protocol_t>
void btree_store_t<protocol_t>::set_sindexes(
        const std::map<std::string, secondary_index_t> &sindexes,
        buf_lock_t *sindex_block,
        value_sizer_t<void> *sizer,
        value_deleter_t *deleter,
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

            guarantee(std_contains(secondary_index_slices, it->first));
            clear_sindex(sindex_block->txn(), it->second.superblock,
                         sizer, deleter, interruptor);
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
        buf_lock_t *sindex_block,
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
                     sizer, deleter, interruptor);
        secondary_index_slices.erase(id);
    }
    return true;
}

template <class protocol_t>
MUST_USE bool btree_store_t<protocol_t>::acquire_sindex_superblock_for_read(
        const std::string &id,
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
        throw sindex_not_post_constructed_exc_t(id);
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
        throw sindex_not_post_constructed_exc_t(id);
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
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
    assert_thread();

    /* Get the sindex block. */
    buf_lock_t sindex_block = acquire_sindex_block_for_write(parent, sindex_block_id);

    acquire_all_sindex_superblocks_for_write(&sindex_block, sindex_sbs_out);
}

template <class protocol_t>
void btree_store_t<protocol_t>::acquire_all_sindex_superblocks_for_write(
        buf_lock_t *sindex_block,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t) {
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

template <class protocol_t>
bool btree_store_t<protocol_t>::acquire_sindex_superblocks_for_write(
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
