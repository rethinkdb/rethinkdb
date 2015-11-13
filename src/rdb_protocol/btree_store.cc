// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/store.hpp"  // NOLINT(build/include_order)

#include <functional>  // NOLINT(build/include_order)

#include "arch/runtime/coroutines.hpp"
#include "btree/depth_first_traversal.hpp"
#include "btree/node.hpp"
#include "btree/operations.hpp"
#include "btree/reql_specific.hpp"
#include "btree/secondary_operations.hpp"
#include "buffer_cache/alt.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/buffer_stream.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/archive/versioned.hpp"
#include "containers/disk_backed_queue.hpp"
#include "containers/scoped.hpp"
#include "logger.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/erase_range.hpp"
#include "rdb_protocol/protocol.hpp"
#include "stl_utils.hpp"

// The maximal number of writes that can be in line for a superblock acquisition
// at a time (including the write that's currently holding the superblock, if any).
// This is to throttle writes compared to reads.
//
// Note: We don't currently have a similar semaphore for reads.
//  If we actually wanted to control the ratio between reads and writes we could
//  add one for that purpose. For the time being a semaphore that throttles write
//  acquisitions of the superblock is likely enough. The rationale behind this
//  asymmetry is that writes can be fired in huge numbers in parallel (e.g. during a
//  data import), while reads - in most applications - are pretty much serialized
//  since the application has to wait on the result of the read.
//  Thus we need to throttle writes, but can probably get away without throttling
//  reads here.
//  ... also long-running read transactions usually use a snapshot, so they don't
//  block out writes anyway.
const int64_t WRITE_SUPERBLOCK_ACQ_WAITERS_LIMIT = 2;

// Some of this implementation is in store.cc and some in btree_store.cc for no
// particularly good reason.  Historically it turned out that way, and for now
// there's not enough refactoring urgency to combine them into one.
//
// (Really, some of the implementation is also in rdb_protocol/btree.cc.)

sindex_not_ready_exc_t::sindex_not_ready_exc_t(
        std::string sindex_name,
        const secondary_index_t &sindex,
        const std::string &table_name) {
    if (sindex.being_deleted) {
        rassert(false, "A query tried to access index `%s` on table `%s` which is "
                       "currently being deleted. Queries should not be able to "
                       "access such indexes in the first place, so this is a bug.",
                sindex_name.c_str(),
                table_name.c_str());
        info = strprintf("Index `%s` on table `%s` "
                         "was accessed while it was being deleted.",
                         sindex_name.c_str(),
                         table_name.c_str());
    } else {
        rassert(!sindex.post_construction_complete());
        info = strprintf("Index `%s` on table `%s` "
                         "was accessed before its construction was finished.",
                         sindex_name.c_str(),
                         table_name.c_str());
    }
}

const char* sindex_not_ready_exc_t::what() const throw() {
    return info.c_str();
}

sindex_not_ready_exc_t::~sindex_not_ready_exc_t() throw() { }

store_t::store_t(const region_t &region,
                 serializer_t *serializer,
                 cache_balancer_t *balancer,
                 const std::string &perfmon_name,
                 bool create,
                 perfmon_collection_t *parent_perfmon_collection,
                 rdb_context_t *_ctx,
                 io_backender_t *io_backender,
                 const base_path_t &base_path,
                 namespace_id_t _table_id,
                 update_sindexes_t update_sindexes)
    : store_view_t(region),
      perfmon_collection(),
      io_backender_(io_backender), base_path_(base_path),
      perfmon_collection_membership(parent_perfmon_collection, &perfmon_collection, perfmon_name),
      ctx(_ctx),
      table_id(_table_id),
      write_superblock_acq_semaphore(WRITE_SUPERBLOCK_ACQ_WAITERS_LIMIT)
{
    cache.init(new cache_t(serializer, balancer, &perfmon_collection));
    general_cache_conn.init(new cache_conn_t(cache.get()));

    if (create) {
        vector_stream_t key;
        // The version used when deserializing this data depends on the block magic.
        // The block magic set by init_real_superblock corresponds to the latest version
        // and so this serialization does too.
        // VSI: Do this better.
        write_message_t wm;
        region_t kr = region_t::universe();
        serialize_for_metainfo(&wm, kr);
        key.reserve(wm.size());
        int res = send_write_message(&key, &wm);
        guarantee(!res);

        txn_t txn(general_cache_conn.get(), write_durability_t::HARD, 1);
        buf_lock_t sb_lock(&txn, SUPERBLOCK_ID, alt_create_t::create);
        real_superblock_t superblock(std::move(sb_lock));
        btree_slice_t::init_real_superblock(&superblock, key.vector(), binary_blob_t());
    }

    btree.init(new btree_slice_t(cache.get(),
                                 &perfmon_collection,
                                 "primary",
                                 index_type_t::PRIMARY));

    // Initialize sindex slices and metainfo
    {
        // Since this is the btree constructor, nothing else should be locking these
        // things yet, so this should work fairly quickly and does not need a real
        // interruptor.
        cond_t dummy_interruptor;
        read_token_t token;
        new_read_token(&token);
        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        acquire_superblock_for_read(&token,
                                    &txn,
                                    &superblock,
                                    &dummy_interruptor,
                                    false /* don't use snapshot */ );

        metainfo.init(new store_metainfo_manager_t(superblock.get()));

        buf_lock_t sindex_block(superblock->expose_buf(),
                                superblock->get_sindex_block_id(),
                                access_t::read);

        std::map<sindex_name_t, secondary_index_t> sindexes;
        get_secondary_indexes(&sindex_block, &sindexes);

        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            // Deleted secondary indexes should not be added to the perfmons
            perfmon_collection_t *pc =
                it->first.being_deleted
                ? NULL
                : &perfmon_collection;
            auto slice = make_scoped<btree_slice_t>(cache.get(),
                                                    pc,
                                                    it->first.name,
                                                    index_type_t::SECONDARY);
            secondary_index_slices.insert(std::make_pair(it->second.id,
                                                         std::move(slice)));
        }
    }

    switch (update_sindexes) {
    case update_sindexes_t::UPDATE:
        help_construct_bring_sindexes_up_to_date();
        break;
    case update_sindexes_t::LEAVE_ALONE:
        break;
    default:
        unreachable();
    }
}

store_t::~store_t() {
    assert_thread();
    drainer.drain();
}

void store_t::read(
        DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
        const read_t &read,
        read_response_t *response,
        read_token_t *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;

    acquire_superblock_for_read(token, &txn, &superblock,
                                interruptor,
                                read.use_snapshot());
    DEBUG_ONLY_CODE(metainfo->visit(
        superblock.get(), metainfo_checker.region, metainfo_checker.callback));
    protocol_read(read, response, superblock.get(), interruptor);
}

void store_t::write(
        DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
        const region_map_t<binary_blob_t>& new_metainfo,
        const write_t &write,
        write_response_t *response,
        const write_durability_t durability,
        state_timestamp_t timestamp,
        UNUSED order_token_t order_token,  // TODO
        write_token_t *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> real_superblock;
    // We assume one block per document, plus changes to the stats block and superblock.
    const int expected_change_count = 2 + write.expected_document_changes();
    acquire_superblock_for_write(expected_change_count, durability, token,
                                 &txn, &real_superblock, interruptor);
    DEBUG_ONLY_CODE(metainfo->visit(
        real_superblock.get(), metainfo_checker.region, metainfo_checker.callback));
    metainfo->update(real_superblock.get(), new_metainfo);
    protocol_write(write, response, timestamp, &real_superblock, interruptor);
}

void store_t::reset_data(
        const binary_blob_t &zero_metainfo,
        const region_t &subregion,
        const write_durability_t durability,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    guarantee(subregion.beg == get_region().beg && subregion.end == get_region().end);
    assert_thread();
    with_priority_t p(CORO_PRIORITY_RESET_DATA);

    // Erase the data in small chunks
    always_true_key_tester_t key_tester;
    const uint64_t max_erased_per_pass = 100;
    for (continue_bool_t done_erasing = continue_bool_t::CONTINUE;
         done_erasing == continue_bool_t::CONTINUE;) {
        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;

        const int expected_change_count = 2 + max_erased_per_pass;
        write_token_t token;
        new_write_token(&token);
        acquire_superblock_for_write(expected_change_count,
                                     durability,
                                     &token,
                                     &txn,
                                     &superblock,
                                     interruptor);

        buf_lock_t sindex_block(superblock->expose_buf(),
                                superblock->get_sindex_block_id(),
                                access_t::write);

        /* Note we don't allow interruption during this step; it's too easy to end up in
        an inconsistent state. */
        cond_t non_interruptor;

        rdb_live_deletion_context_t deletion_context;
        std::vector<rdb_modification_report_t> mod_reports;
        key_range_t deleted_range;
        done_erasing = rdb_erase_small_range(btree.get(),
                                             &key_tester,
                                             subregion.inner,
                                             superblock.get(),
                                             &deletion_context,
                                             &non_interruptor,
                                             max_erased_per_pass,
                                             &mod_reports,
                                             &deleted_range);

        region_t deleted_region(subregion.beg, subregion.end, deleted_range);
        metainfo->update(superblock.get(),
                         region_map_t<binary_blob_t>(deleted_region, zero_metainfo));

        superblock.reset();
        if (!mod_reports.empty()) {
            update_sindexes(txn.get(), &sindex_block, mod_reports, true);
        }
    }
}

std::map<std::string, std::pair<sindex_config_t, sindex_status_t> > store_t::sindex_list(
        UNUSED signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    scoped_ptr_t<real_superblock_t> superblock;
    scoped_ptr_t<txn_t> txn;
    get_btree_superblock_and_txn_for_reading(general_cache_conn.get(),
        CACHE_SNAPSHOTTED_NO, &superblock, &txn);
    buf_lock_t sindex_block(superblock->expose_buf(),
                            superblock->get_sindex_block_id(),
                            access_t::read);
    superblock->release();

    std::map<sindex_name_t, secondary_index_t> secondary_indexes;
    get_secondary_indexes(&sindex_block, &secondary_indexes);
    sindex_block.reset_buf_lock();

    std::map<std::string, std::pair<sindex_config_t, sindex_status_t> > results;
    for (const auto &pair : secondary_indexes) {
        guarantee(pair.first.being_deleted == pair.second.being_deleted);
        if (pair.second.being_deleted) {
            continue;
        }
        std::pair<sindex_config_t, sindex_status_t> *res = &results[pair.first.name];
        sindex_disk_info_t disk_info;
        try {
            deserialize_sindex_info_or_crash(pair.second.opaque_definition, &disk_info);
        } catch (const archive_exc_t &) {
            crash("corrupted sindex definition");
        }

        res->first.func = disk_info.mapping;
        res->first.func_version = disk_info.mapping_version_info.original_reql_version;
        res->first.multi = disk_info.multi;
        res->first.geo = disk_info.geo;

        res->second.outdated =
            disk_info.mapping_version_info.latest_compatible_reql_version !=
                reql_version_t::LATEST;
        if (pair.second.is_ready()) {
            res->second.ready = true;
            res->second.progress_numerator = 0.0;
            res->second.progress_denominator = 0.0;
            res->second.start_time = -1;
        } else {
            res->second.ready = false;
            res->second.progress_numerator = get_sindex_progress(pair.second.id);
            res->second.progress_denominator = 1.0;
            res->second.start_time = get_sindex_start_time(pair.second.id);
        }
    }

    return results;
}

void store_t::sindex_create(
        const std::string &name,
        const sindex_config_t &config,
        UNUSED signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    scoped_ptr_t<real_superblock_t> superblock;
    scoped_ptr_t<txn_t> txn;
    get_btree_superblock_and_txn_for_writing(general_cache_conn.get(),
        &write_superblock_acq_semaphore, write_access_t::write, 1,
        write_durability_t::HARD, &superblock, &txn);
    buf_lock_t sindex_block(superblock->expose_buf(),
                            superblock->get_sindex_block_id(),
                            access_t::write);
    superblock->release();

    /* Note that this function allows creating sindexes with older ReQL versions. For
    example, suppose that the user upgrades to a newer version of RethinkDB, and then
    they want to add a replica to a table with an outdated secondary index. Then this
    function would be called to create the outdated secondary index on the new replica.
    */
    sindex_reql_version_info_t version_info;
    version_info.original_reql_version = config.func_version;
    version_info.latest_compatible_reql_version = config.func_version;
    version_info.latest_checked_reql_version = reql_version_t::LATEST;
    sindex_disk_info_t info(config.func, version_info, config.multi, config.geo);

    write_message_t wm;
    serialize_sindex_info(&wm, info);
    vector_stream_t stream;
    stream.reserve(wm.size());
    int write_res = send_write_message(&stream, &wm);
    guarantee(write_res == 0);

    sindex_name_t sindex_name(name);
    boost::optional<uuid_u> sindex_id = add_sindex_internal(
        sindex_name, stream.vector(), &sindex_block);
    guarantee(sindex_id, "sindex_create() called with a sindex name that exists");

    // Kick off index post construction
    coro_t::spawn_sometime(std::bind(&rdb_protocol::resume_construct_sindex,
                                     *sindex_id,
                                     key_range_t::universe(),
                                     this,
                                     drainer.lock()));
}

void store_t::sindex_rename_multi(
        const std::map<std::string, std::string> &name_changes,
        UNUSED signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    scoped_ptr_t<real_superblock_t> superblock;
    scoped_ptr_t<txn_t> txn;
    get_btree_superblock_and_txn_for_writing(general_cache_conn.get(),
        &write_superblock_acq_semaphore, write_access_t::write, 1,
        write_durability_t::HARD, &superblock, &txn);
    buf_lock_t sindex_block(superblock->expose_buf(),
                            superblock->get_sindex_block_id(),
                            access_t::write);
    superblock->release();

    /* First we remove all the secondary indexes and hide their perfmons, but put the
    definitions and `btree_stats_t`s into `to_put_back` indexed by their new names. Then
    we go through and put them all back under their new names. */
    std::map<std::string, std::pair<secondary_index_t, btree_stats_t *> > to_put_back;

    for (const auto &pair : name_changes) {
        secondary_index_t definition;
        bool success = get_secondary_index(
            &sindex_block, sindex_name_t(pair.first), &definition);
        guarantee(success);
        success = delete_secondary_index(&sindex_block, sindex_name_t(pair.first));
        guarantee(success);

        auto slice_it = secondary_index_slices.find(definition.id);
        guarantee(slice_it != secondary_index_slices.end());
        guarantee(slice_it->second.has());
        slice_it->second->assert_thread();
        btree_stats_t *stats = &slice_it->second->stats;
        stats->hide();

        to_put_back.insert(std::make_pair(
            pair.second, std::make_pair(definition, stats)));
    }

    for (const auto &pair : to_put_back) {
        set_secondary_index(&sindex_block, sindex_name_t(pair.first), pair.second.first);
        pair.second.second->rename(&perfmon_collection, "index-" + pair.first);
    }
}

void store_t::sindex_drop(
        const std::string &name,
        UNUSED signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    scoped_ptr_t<real_superblock_t> superblock;
    scoped_ptr_t<txn_t> txn;
    get_btree_superblock_and_txn_for_writing(general_cache_conn.get(),
        &write_superblock_acq_semaphore, write_access_t::write, 1,
        write_durability_t::HARD, &superblock, &txn);
    buf_lock_t sindex_block(superblock->expose_buf(),
                            superblock->get_sindex_block_id(),
                            access_t::write);
    superblock->release();

    secondary_index_t sindex;
    bool success = ::get_secondary_index(&sindex_block, sindex_name_t(name), &sindex);
    guarantee(success, "sindex_drop() called on sindex that doesn't exist");

    /* Mark the secondary index as deleted */
    success = mark_secondary_index_deleted(&sindex_block, sindex_name_t(name));
    guarantee(success);

    /* Clear the sindex later. It starts its own transaction and we don't
    want to deadlock because we're still holding locks. */
    coro_t::spawn_sometime(std::bind(&store_t::delayed_clear_and_drop_sindex,
                                     this,
                                     sindex,
                                     drainer.lock()));
}

new_mutex_in_line_t store_t::get_in_line_for_sindex_queue(buf_lock_t *sindex_block) {
    assert_thread();
    // The line for the sindex queue is there to guarantee that we push things to
    // the sindex queue(s) in the same order in which we held the sindex block.
    // It allows us to release the sindex block before starting to push data to the
    // sindex queues without sacrificing that guarantee.
    // Pushing data to those queues can take a while, and we wouldn't want to block
    // other transactions while we are doing that.

    guarantee(!sindex_block->empty());
    guarantee(sindex_block->access() == access_t::write);
    sindex_block->write_acq_signal()->wait();
    return new_mutex_in_line_t(&sindex_queue_mutex);
}

rwlock_in_line_t store_t::get_in_line_for_cfeed_stamp(access_t access) {
    return rwlock_in_line_t(&cfeed_stamp_lock, access);
}


void store_t::register_sindex_queue(
            disk_backed_queue_wrapper_t<rdb_modification_report_t> *disk_backed_queue,
            const key_range_t &construction_range,
            const new_mutex_in_line_t *acq) {
    assert_thread();
    acq->acq_signal()->wait_lazily_unordered();

    for (const auto &q : sindex_queues) {
        guarantee(q.queue != disk_backed_queue);
    }
    sindex_queues.push_back(ranged_sindex_queue_t{construction_range,
                                                  disk_backed_queue});
}

void store_t::deregister_sindex_queue(
            disk_backed_queue_wrapper_t<rdb_modification_report_t> *disk_backed_queue,
            const new_mutex_in_line_t *acq) {
    assert_thread();
    acq->acq_signal()->wait_lazily_unordered();

    for (auto it = sindex_queues.begin(); it != sindex_queues.end(); ++it) {
        if (it->queue == disk_backed_queue) {
            sindex_queues.erase(it);
            return;
        }
    }
    unreachable();
}

void store_t::emergency_deregister_sindex_queue(
    disk_backed_queue_wrapper_t<rdb_modification_report_t> *disk_backed_queue) {
    assert_thread();
    drainer.assert_draining();
    new_mutex_in_line_t acq(&sindex_queue_mutex);
    acq.acq_signal()->wait_lazily_unordered();

    deregister_sindex_queue(disk_backed_queue, &acq);
}

void store_t::update_sindexes(
            txn_t *txn,
            buf_lock_t *sindex_block,
            const std::vector<rdb_modification_report_t> &mod_reports,
            bool release_sindex_block) {
    new_mutex_in_line_t acq = get_in_line_for_sindex_queue(sindex_block);
    {
        sindex_access_vector_t sindexes;
        acquire_all_sindex_superblocks_for_write(sindex_block, &sindexes);
        if (release_sindex_block) {
            sindex_block->reset_buf_lock();
        }

        rdb_live_deletion_context_t deletion_context;
        for (size_t i = 0; i < mod_reports.size(); ++i) {
            rdb_update_sindexes(this,
                                sindexes,
                                &mod_reports[i],
                                txn,
                                &deletion_context,
                                NULL,
                                NULL,
                                NULL);
        }
    }

    // Write mod reports onto the sindex queue. We are in line for the
    // sindex_queue mutex and can already release all other locks.
    sindex_queue_push(mod_reports, &acq);
}

void store_t::sindex_queue_push(const rdb_modification_report_t &mod_report,
                                const new_mutex_in_line_t *acq) {
    assert_thread();
    acq->acq_signal()->wait_lazily_unordered();

    for (auto it = sindex_queues.begin(); it != sindex_queues.end(); ++it) {
        if (it->construction_range.contains_key(mod_report.primary_key)) {
            it->queue->push(mod_report);
        }
    }
}

void store_t::sindex_queue_push(
        const std::vector<rdb_modification_report_t> &mod_reports,
        const new_mutex_in_line_t *acq) {
    assert_thread();
    acq->acq_signal()->wait_lazily_unordered();

    for (size_t i = 0; i < mod_reports.size(); ++i) {
        for (auto it = sindex_queues.begin(); it != sindex_queues.end(); ++it) {
            if (it->construction_range.contains_key(mod_reports[i].primary_key)) {
                it->queue->push(mod_reports[i]);
            }
        }
    }
}

double store_t::get_sindex_progress(uuid_u const &id) {
    auto iterator = sindex_context.find(id);
    if (iterator == sindex_context.end()) {
        return 0.0;
    } else {
        return *iterator->second.second;
    }
}

microtime_t store_t::get_sindex_start_time(uuid_u const &id) {
    auto iterator = sindex_context.find(id);
    if (iterator == sindex_context.end()) {
        return -1;
    } else {
        return iterator->second.first;
    }
}

boost::optional<uuid_u> store_t::add_sindex_internal(
        const sindex_name_t &name,
        const std::vector<char> &opaque_definition,
        buf_lock_t *sindex_block) {
    secondary_index_t sindex;
    if (::get_secondary_index(sindex_block, name, &sindex)) {
        return boost::none; // sindex was already created
    } else {
        {
            buf_lock_t sb_lock(sindex_block, alt_create_t::create);
            sindex.superblock = sb_lock.block_id();
            sindex.opaque_definition = opaque_definition;

            sindex_superblock_t superblock(std::move(sb_lock));
            btree_slice_t::init_sindex_superblock(&superblock);
        }

        secondary_index_slices.insert(
                std::make_pair(sindex.id,
                               make_scoped<btree_slice_t>(cache.get(),
                                                          &perfmon_collection,
                                                          name.name,
                                                          index_type_t::SECONDARY)));

        sindex.needs_post_construction_range = key_range_t::universe();

        ::set_secondary_index(sindex_block, name, sindex);
        return sindex.id;
    }
}

sindex_name_t compute_sindex_deletion_name(uuid_u sindex_uuid) {
    sindex_name_t result("_DEL_" + uuid_to_str(sindex_uuid));
    result.being_deleted = true;
    return result;
}

class clear_sindex_traversal_cb_t
        : public depth_first_traversal_callback_t {
public:
    explicit clear_sindex_traversal_cb_t(const key_range_t &pkey_rng)
        : pkey_range(pkey_rng),
          num_traversed(0),
          last_traversed_key(store_key_t::min()) {
        collected_keys.reserve(CHUNK_SIZE);
    }
    continue_bool_t handle_pair(scoped_key_value_t &&keyvalue, signal_t *) {
        store_key_t key(keyvalue.key());
        // Skip keys that are not in the given primary key range.
        if (pkey_range.contains_key(
                ql::datum_t::extract_primary(key))) {
            collected_keys.push_back(store_key_t(keyvalue.key()));
        }
        ++num_traversed;
        rassert(key > last_traversed_key);
        last_traversed_key = key;
        if (num_traversed >= CHUNK_SIZE) {
            return continue_bool_t::ABORT;
        } else {
            return continue_bool_t::CONTINUE;
        }
    }
    const std::vector<store_key_t> &get_keys() const {
        return collected_keys;
    }
    store_key_t get_last_traversed_key() const {
        return last_traversed_key;
    }
    static const size_t CHUNK_SIZE = 32;
private:
    key_range_t pkey_range;
    std::vector<store_key_t> collected_keys;
    size_t num_traversed;
    store_key_t last_traversed_key;
};

void store_t::clear_sindex_data(
        uuid_u sindex_id,
        value_sizer_t *sizer,
        const deletion_context_t *deletion_context,
        const key_range_t &pkey_range_to_clear,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    /* Delete one piece of the secondary index at a time */
    key_range_t remaining_range = key_range_t::universe();
    for (bool reached_end = false; !reached_end;)
    {
        coro_t::yield();

        /* Start a write transaction. */
        write_token_t token;
        new_write_token(&token);
        scoped_ptr_t<txn_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        acquire_superblock_for_write(
            // Not really the right value, since many keys will share a leaf node:
            clear_sindex_traversal_cb_t::CHUNK_SIZE,
            write_durability_t::SOFT,
            &token,
            &txn,
            &superblock,
            interruptor);

        /* Get the sindex block. */
        buf_lock_t sindex_block(superblock->expose_buf(),
                                superblock->get_sindex_block_id(),
                                access_t::write);
        superblock->release();

        secondary_index_t sindex;
        bool found = get_secondary_index(&sindex_block, sindex_id, &sindex);
        if (!found) {
            // The index got dropped by someone else. That's ok, there's nothing left
            // to do for us.
            return;
        }

        /* Clear part of the index data */
        buf_lock_t sindex_superblock_lock(buf_parent_t(&sindex_block),
                                          sindex.superblock, access_t::write);
        sindex_block.reset_buf_lock();
        scoped_ptr_t<sindex_superblock_t> sindex_superblock
            = make_scoped<sindex_superblock_t>(std::move(sindex_superblock_lock));

        /* 1. Collect a bunch of keys to delete */
        clear_sindex_traversal_cb_t traversal_cb(pkey_range_to_clear);
        reached_end = (continue_bool_t::CONTINUE == btree_depth_first_traversal(
            sindex_superblock.get(),
            remaining_range,
            &traversal_cb,
            access_t::read,
            direction_t::FORWARD,
            release_superblock_t::KEEP,
            interruptor));

        /* Update the traversal range so we don't traverse the same range again */
        remaining_range = key_range_t(
            key_range_t::open,
            traversal_cb.get_last_traversed_key(),
            key_range_t::none,
            store_key_t());

        /* 2. Actually delete them */
        const std::vector<store_key_t> &keys = traversal_cb.get_keys();
        for (size_t i = 0; i < keys.size(); ++i) {
            promise_t<superblock_t *> superblock_promise;
            {
                keyvalue_location_t kv_location;
                find_keyvalue_location_for_write(sizer, sindex_superblock.release(),
                        keys[i].btree_key(), repli_timestamp_t::distant_past,
                        deletion_context->balancing_detacher(),
                        &kv_location, NULL /* trace */,
                        &superblock_promise);

                if (kv_location.there_originally_was_value) {
                    deletion_context->in_tree_deleter()->delete_value(
                        buf_parent_t(&kv_location.buf), kv_location.value.get());
                    kv_location.value.reset();

                    buf_write_t write(&kv_location.buf);
                    auto leaf_node = static_cast<leaf_node_t *>(write.get_data_write());
                    leaf::remove(sizer,
                                 leaf_node,
                                 keys[i].btree_key(),
                                 repli_timestamp_t::distant_past,
                                 kv_location.buf.get_recency(),
                                 key_modification_proof_t::real_proof());
                }
                check_and_handle_underfull(sizer, &kv_location.buf,
                        &kv_location.last_buf, kv_location.superblock,
                        keys[i].btree_key(),
                        deletion_context->balancing_detacher());

                /* Here kv_location is destructed, which returns the superblock */
            }

            /* Reclaim the sindex superblock for the next deletion */
            sindex_superblock.init(static_cast<sindex_superblock_t *>(
                superblock_promise.wait()));
        }
    }
}

void store_t::drop_sindex(uuid_u sindex_id) THROWS_NOTHING {
    /* Start a transaction. */
    write_token_t token;
    new_write_token(&token);
    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    cond_t non_interruptor;
    acquire_superblock_for_write(
        2,
        write_durability_t::SOFT,
        &token,
        &txn,
        &superblock,
        &non_interruptor);

    /* Get the sindex block. */
    buf_lock_t sindex_block(superblock->expose_buf(),
                            superblock->get_sindex_block_id(),
                            access_t::write);
    superblock->release();

    secondary_index_t sindex;
    bool found = get_secondary_index(&sindex_block, sindex_id, &sindex);
    if (!found) {
        // The index got dropped by someone else. That's ok, there's nothing left for us
        // to do.
        return;
    }

    /* The root node should have been emptied at this point. Delete it */
    {
        buf_lock_t sindex_superblock_lock(buf_parent_t(&sindex_block),
                                          sindex.superblock, access_t::write);
        sindex_superblock_t sindex_superblock(std::move(sindex_superblock_lock));
        if (sindex_superblock.get_root_block_id() != NULL_BLOCK_ID) {
            buf_lock_t root_node(sindex_superblock.expose_buf(),
                                 sindex_superblock.get_root_block_id(),
                                 access_t::write);
            {
                /* Guarantee that the root is actually empty. */
                buf_read_t rread(&root_node);
                const node_t *node = static_cast<const node_t *>(
                        rread.get_data_read());
                if (node::is_internal(node)) {
                    const internal_node_t *root_int_node
                        = static_cast<const internal_node_t *>(
                            rread.get_data_read());
                    /* This just prints a warning in release mode for now,
                    because leaking a few blocks is probably better than
                    making the database inaccessible. */
                    rassert(root_int_node->npairs == 0);
                    if (root_int_node->npairs != 0) {
                        logWRN("The secondary index tree was not empty after "
                               "clearing it. We are leaking some data blocks. "
                               "Please report this issue at "
                               "https://github.com/rethinkdb/rethinkdb/issues/");
                    }
                }
                /* If the root is a leaf we are good */
            }
            /* Delete the root */
            root_node.write_acq_signal()->wait_lazily_unordered();
            root_node.mark_deleted();
        }
        /* Under normal circumstances, sindex superblocks do not have stat or sindex
        blocks. However, we used to create stat and sindex blocks, so some very old
        secondary indexes may still have them. Here we check for them and delete them
        if they are present. */
        if (sindex_superblock.get_stat_block_id() != NULL_BLOCK_ID) {
            buf_lock_t stat_block(sindex_superblock.expose_buf(),
                                  sindex_superblock.get_stat_block_id(),
                                  access_t::write);
            stat_block.write_acq_signal()->wait_lazily_unordered();
            stat_block.mark_deleted();
        }
        if (sindex_superblock.get_sindex_block_id() != NULL_BLOCK_ID) {
            buf_lock_t sind_block(sindex_superblock.expose_buf(),
                                  sindex_superblock.get_sindex_block_id(),
                                  access_t::write);
            sind_block.write_acq_signal()->wait_lazily_unordered();
            sind_block.mark_deleted();
        }
    }
    /* Now it's safe to completely delete the index */
    buf_lock_t sindex_superblock_lock(buf_parent_t(&sindex_block),
                                      sindex.superblock, access_t::write);
    sindex_superblock_lock.write_acq_signal()->wait_lazily_unordered();
    sindex_superblock_lock.mark_deleted();
    ::delete_secondary_index(&sindex_block, compute_sindex_deletion_name(sindex.id));
    size_t num_erased = secondary_index_slices.erase(sindex.id);
    guarantee(num_erased == 1);
}

bool secondary_indexes_are_equivalent(const std::vector<char> &left,
                                      const std::vector<char> &right) {
    sindex_disk_info_t sindex_info_left;
    sindex_disk_info_t sindex_info_right;
    deserialize_sindex_info_or_crash(left, &sindex_info_left);
    deserialize_sindex_info_or_crash(right, &sindex_info_right);

    if (sindex_info_left.multi == sindex_info_right.multi &&
        sindex_info_left.geo == sindex_info_right.geo &&
        sindex_info_left.mapping_version_info.original_reql_version ==
            sindex_info_right.mapping_version_info.original_reql_version) {
        // Need to determine if the mapping function is the same, re-serialize them
        // and compare the vectors
        bool res;
        write_message_t wm_left;
        vector_stream_t stream_left;
        serialize_for_version(cluster_version_t::CLUSTER, &wm_left,
                              sindex_info_left.mapping);
        res = send_write_message(&stream_left, &wm_left);
        guarantee(res == 0);

        write_message_t wm_right;
        vector_stream_t stream_right;
        serialize_for_version(cluster_version_t::CLUSTER, &wm_right,
                              sindex_info_right.mapping);
        res = send_write_message(&stream_right, &wm_right);
        guarantee(res == 0);

        return stream_left.vector() == stream_right.vector();
    }

    return false;
}

std::map<sindex_name_t, secondary_index_t> store_t::get_sindexes() const {
    assert_thread();

    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    get_btree_superblock_and_txn_for_reading(
        general_cache_conn.get(), CACHE_SNAPSHOTTED_NO, &superblock, &txn);

    buf_lock_t sindex_block(
        superblock->expose_buf(), superblock->get_sindex_block_id(), access_t::read);

    std::map<sindex_name_t, secondary_index_t> sindexes;
    get_secondary_indexes(&sindex_block, &sindexes);

    return sindexes;
}

bool store_t::mark_index_up_to_date(uuid_u id,
                                    buf_lock_t *sindex_block,
                                    const key_range_t &except_for_remaining_range)
    THROWS_NOTHING {
    secondary_index_t sindex;
    bool found = ::get_secondary_index(sindex_block, id, &sindex);

    if (found) {
        sindex.needs_post_construction_range = except_for_remaining_range;

        ::set_secondary_index(sindex_block, id, sindex);
    }

    return found;
}

MUST_USE bool store_t::mark_secondary_index_deleted(
        buf_lock_t *sindex_block,
        const sindex_name_t &name) {
    secondary_index_t sindex;
    bool success = get_secondary_index(sindex_block, name, &sindex);
    if (!success) {
        return false;
    }

    // Delete the current entry
    success = delete_secondary_index(sindex_block, name);
    guarantee(success);

    // Insert the new entry under a different name
    const sindex_name_t sindex_del_name = compute_sindex_deletion_name(sindex.id);
    sindex.being_deleted = true;
    set_secondary_index(sindex_block, sindex_del_name, sindex);

    // Hide the index from the perfmon collection
    auto slice_it = secondary_index_slices.find(sindex.id);
    guarantee(slice_it != secondary_index_slices.end());
    guarantee(slice_it->second.has());
    slice_it->second->assert_thread();
    slice_it->second->stats.hide();

    return true;
}

MUST_USE bool store_t::acquire_sindex_superblock_for_read(
        const sindex_name_t &name,
        const std::string &table_name,
        real_superblock_t *superblock,
        scoped_ptr_t<sindex_superblock_t> *sindex_sb_out,
        std::vector<char> *opaque_definition_out,
        uuid_u *sindex_uuid_out)
    THROWS_ONLY(sindex_not_ready_exc_t) {
    assert_thread();
    rassert(opaque_definition_out != NULL);
    rassert(sindex_uuid_out != NULL);

    /* Acquire the sindex block. */
    buf_lock_t sindex_block(superblock->expose_buf(), superblock->get_sindex_block_id(),
                            access_t::read);
    superblock->release();

    /* Figure out what the superblock for this index is. */
    secondary_index_t sindex;
    if (!::get_secondary_index(&sindex_block, name, &sindex)) {
        return false;
    }

    *opaque_definition_out = sindex.opaque_definition;
    *sindex_uuid_out = sindex.id;

    if (!sindex.is_ready()) {
        throw sindex_not_ready_exc_t(name.name, sindex, table_name);
    }

    buf_lock_t superblock_lock(&sindex_block, sindex.superblock, access_t::read);
    sindex_block.reset_buf_lock();
    sindex_sb_out->init(new sindex_superblock_t(std::move(superblock_lock)));
    return true;
}

MUST_USE bool store_t::acquire_sindex_superblock_for_write(
        const sindex_name_t &name,
        const std::string &table_name,
        real_superblock_t *superblock,
        scoped_ptr_t<sindex_superblock_t> *sindex_sb_out,
        uuid_u *sindex_uuid_out)
    THROWS_ONLY(sindex_not_ready_exc_t) {
    assert_thread();
    rassert(sindex_uuid_out != NULL);

    /* Get the sindex block. */
    buf_lock_t sindex_block(superblock->expose_buf(),
                            superblock->get_sindex_block_id(),
                            access_t::write);
    superblock->release();

    /* Figure out what the superblock for this index is. */
    secondary_index_t sindex;
    if (!::get_secondary_index(&sindex_block, name, &sindex)) {
        return false;
    }
    *sindex_uuid_out = sindex.id;

    if (!sindex.is_ready()) {
        throw sindex_not_ready_exc_t(name.name, sindex, table_name);
    }


    buf_lock_t superblock_lock(&sindex_block, sindex.superblock,
                               access_t::write);
    sindex_block.reset_buf_lock();
    sindex_sb_out->init(new sindex_superblock_t(std::move(superblock_lock)));
    return true;
}

store_t::sindex_access_t::sindex_access_t(btree_slice_t *_btree,
                                          sindex_name_t _name,
                                          secondary_index_t _sindex,
                                          scoped_ptr_t<sindex_superblock_t> _superblock)
    : btree(_btree),
      name(std::move(_name)),
      sindex(std::move(_sindex)),
      superblock(std::move(_superblock))
{ }

store_t::sindex_access_t::~sindex_access_t() { }


void store_t::acquire_all_sindex_superblocks_for_write(
        block_id_t sindex_block_id,
        buf_parent_t parent,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_ready_exc_t) {
    assert_thread();

    /* Get the sindex block. */
    buf_lock_t sindex_block(parent, sindex_block_id, access_t::write);

    acquire_all_sindex_superblocks_for_write(&sindex_block, sindex_sbs_out);
}

void store_t::acquire_all_sindex_superblocks_for_write(
        buf_lock_t *sindex_block,
        sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_ready_exc_t) {
    acquire_sindex_superblocks_for_write(
            boost::none,
            sindex_block,
            sindex_sbs_out);
}

bool store_t::acquire_sindex_superblocks_for_write(
            boost::optional<std::set<uuid_u> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_ready_exc_t) {
    assert_thread();

    std::map<sindex_name_t, secondary_index_t> sindexes;
    ::get_secondary_indexes(sindex_block, &sindexes);

    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (sindexes_to_acquire && !std_contains(*sindexes_to_acquire, it->second.id)) {
            continue;
        }

        /* Getting the slice and asserting we're on the right thread. */
        btree_slice_t *sindex_slice = secondary_index_slices.at(it->second.id).get();
        sindex_slice->assert_thread();

        buf_lock_t superblock_lock(
                sindex_block, it->second.superblock, access_t::write);

        sindex_sbs_out->push_back(
                make_scoped<sindex_access_t>(
                        get_sindex_slice(it->second.id),
                        it->first,
                        it->second,
                        make_scoped<sindex_superblock_t>(std::move(superblock_lock))));
    }

    // return's true if we got all of the sindexes requested.
    return !sindexes_to_acquire || sindex_sbs_out->size() == sindexes_to_acquire->size();
}

region_map_t<binary_blob_t> store_t::get_metainfo(
        UNUSED order_token_t order_token,  // TODO
        read_token_t *token,
        const region_t &region,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_read(token,
                                &txn, &superblock,
                                interruptor,
                                false /* KSI: christ */);
    return metainfo->get(superblock.get(), region);
}

void store_t::set_metainfo(const region_map_t<binary_blob_t> &new_metainfo,
                           UNUSED order_token_t order_token,  // TODO
                           write_token_t *token,
                           write_durability_t durability,
                           signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_write(1,
                                 durability,
                                 token,
                                 &txn,
                                 &superblock,
                                 interruptor);
    metainfo->update(superblock.get(), new_metainfo);
}

cluster_version_t store_t::metainfo_version(read_token_t *token,
                                            signal_t *interruptor) {
    assert_thread();
    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_read(token,
                                &txn, &superblock,
                                interruptor,
                                false /* KSI: christ */);
    return metainfo->get_version(superblock.get());
}

void store_t::migrate_metainfo(
        UNUSED order_token_t order_token, // TODO
        write_token_t *token,
        cluster_version_t from,
        cluster_version_t to,
        const std::function<binary_blob_t(const region_t &, const binary_blob_t &)> &cb,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_write(1,
                                 write_durability_t::HARD,
                                 token,
                                 &txn,
                                 &superblock,
                                 interruptor);
    metainfo->migrate(superblock.get(), from, to, get_region(), cb);
}

void store_t::acquire_superblock_for_read(
        read_token_t *token,
        scoped_ptr_t<txn_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor,
        bool use_snapshot)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(&token->main_read_token);
    if (token->main_read_token.has()) {
        wait_interruptible(token->main_read_token.get(), interruptor);
    }

    cache_snapshotted_t cache_snapshotted =
        use_snapshot ? CACHE_SNAPSHOTTED_YES : CACHE_SNAPSHOTTED_NO;
    get_btree_superblock_and_txn_for_reading(
        general_cache_conn.get(), cache_snapshotted, sb_out, txn_out);
}

void store_t::acquire_superblock_for_write(
        int expected_change_count,
        write_durability_t durability,
        write_token_t *token,
        scoped_ptr_t<txn_t> *txn_out,
        scoped_ptr_t<real_superblock_t> *sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(&token->main_write_token);
    wait_interruptible(token->main_write_token.get(), interruptor);

    get_btree_superblock_and_txn_for_writing(
            general_cache_conn.get(),
            &write_superblock_acq_semaphore,
            write_access_t::write,
            expected_change_count,
            durability,
            sb_out,
            txn_out);
}

/* store_view_t interface */
void store_t::new_read_token(read_token_t *token_out) {
    assert_thread();
    fifo_enforcer_read_token_t token = main_token_source.enter_read();
    token_out->main_read_token.create(&main_token_sink, token);
}

void store_t::new_write_token(write_token_t *token_out) {
    assert_thread();
    fifo_enforcer_write_token_t token = main_token_source.enter_write();
    token_out->main_write_token.create(&main_token_sink, token);
}
