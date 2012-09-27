#include "btree/btree_store.hpp"

#include "btree/operations.hpp"
#include "serializer/config.hpp"
#include "containers/archive/vector_stream.hpp"
#include "concurrency/wait_any.hpp"

template <class protocol_t>
btree_store_t<protocol_t>::btree_store_t(io_backender_t *io_backender,
                                         const std::string& filename,
                                         int64_t cache_target,
                                         bool create,
                                         perfmon_collection_t *parent_perfmon_collection,
                                         typename protocol_t::context_t *) :
    store_view_t<protocol_t>(protocol_t::region_t::universe()),
    perfmon_collection(),
    perfmon_collection_membership(parent_perfmon_collection, &perfmon_collection, filename)
{
    if (create) {

        standard_serializer_t::create(
            io_backender,
            standard_serializer_t::private_dynamic_config_t(filename),
            standard_serializer_t::static_config_t());
    }

    serializer.init(new standard_serializer_t(
        standard_serializer_t::dynamic_config_t(),
        io_backender,
        standard_serializer_t::private_dynamic_config_t(filename),
        &perfmon_collection));

    if (create) {
        mirrored_cache_static_config_t cache_static_config;
        cache_t::create(serializer.get(), &cache_static_config);
    }

    // TODO: Don't specify cache dynamic config here.
    cache_dynamic_config.max_size = cache_target;
    cache_dynamic_config.max_dirty_size = cache_target / 2;
    cache.init(new cache_t(serializer.get(), &cache_dynamic_config, &perfmon_collection));

    if (create) {
        btree_slice_t::create(cache.get());
    }

    btree.init(new btree_slice_t(cache.get(), &perfmon_collection));

    if (create) {
        // Initialize metainfo to an empty `binary_blob_t` because its domain is
        // required to be `protocol_t::region_t::universe()` at all times
        /* Wow, this is a lot of lines of code for a simple concept. Can we do better? */

        scoped_ptr_t<transaction_t> txn;
        scoped_ptr_t<real_superblock_t> superblock;
        order_token_t order_token = order_source.check_in("btree_store_t<" + protocol_t::protocol_name + ">::store_t::store_t");
        order_token = btree->pre_begin_txn_checkpoint_.check_through(order_token);
        get_btree_superblock_and_txn(btree.get(), rwi_write, 1, repli_timestamp_t::invalid, order_token, &superblock, &txn);
        buf_lock_t* sb_buf = superblock->get();
        clear_superblock_metainfo(txn.get(), sb_buf);

        vector_stream_t key;
        write_message_t msg;
        typename protocol_t::region_t kr = protocol_t::region_t::universe();   // `operator<<` needs a non-const reference  // TODO <- what
        msg << kr;
        DEBUG_VAR int res = send_write_message(&key, &msg);
        rassert(!res);
        set_superblock_metainfo(txn.get(), sb_buf, key.vector(), std::vector<char>());
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
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_read(rwi_read, token, &txn, &superblock, interruptor);

    check_metainfo(DEBUG_ONLY(metainfo_checker, ) txn.get(), superblock.get());

    // Ugly hack
    scoped_ptr_t<superblock_t> superblock2;
    superblock2.init(superblock.release());

    protocol_read(read, response, btree.get(), txn.get(), superblock2.get());
}

template <class protocol_t>
void btree_store_t<protocol_t>::write(
        DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
        const metainfo_t& new_metainfo,
        const typename protocol_t::write_t &write,
        typename protocol_t::write_response_t *response,
        transition_timestamp_t timestamp,
        UNUSED order_token_t order_token,  // TODO
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    const int expected_change_count = 2; // FIXME: this is incorrect, but will do for now
    acquire_superblock_for_write(rwi_write, timestamp.to_repli_timestamp(), expected_change_count, token, &txn, &superblock, interruptor);

    check_and_update_metainfo(DEBUG_ONLY(metainfo_checker, ) new_metainfo, txn.get(), superblock.get());

    protocol_write(write, response, timestamp, btree.get(), txn.get(), superblock.get());
}

// TODO: Figure out wtf does the backfill filtering, figure out wtf constricts delete range operations to hit only a certain hash-interval, figure out what filters keys.
template <class protocol_t>
bool btree_store_t<protocol_t>::send_backfill(
        const region_map_t<protocol_t, state_timestamp_t> &start_point,
        send_backfill_callback_t<protocol_t> *send_backfill_cb,
        typename protocol_t::backfill_progress_t *progress,
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_backfill(token, &txn, &superblock, interruptor);

    region_map_t<protocol_t, binary_blob_t> unmasked_metainfo;
    get_metainfo_internal(txn.get(), superblock->get(), &unmasked_metainfo);
    region_map_t<protocol_t, binary_blob_t> metainfo = unmasked_metainfo.mask(start_point.get_domain());
    if (send_backfill_cb->should_backfill(metainfo)) {
        protocol_send_backfill(start_point, send_backfill_cb, superblock.get(), btree.get(), txn.get(), progress, interruptor);
        return true;
    }
    return false;
}

template <class protocol_t>
void btree_store_t<protocol_t>::receive_backfill(
        const typename protocol_t::backfill_chunk_t &chunk,
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    const int expected_change_count = 1; // FIXME: this is probably not correct

    acquire_superblock_for_write(rwi_write, chunk.get_btree_repli_timestamp(), expected_change_count, token, &txn, &superblock, interruptor);

    protocol_receive_backfill(btree.get(), txn.get(), superblock.get(), interruptor, chunk);
}

template <class protocol_t>
void btree_store_t<protocol_t>::reset_data(
        const typename protocol_t::region_t& subregion,
        const metainfo_t &new_metainfo,
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
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
    acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid, expected_change_count, token, &txn, &superblock, interruptor);

    region_map_t<protocol_t, binary_blob_t> old_metainfo;
    get_metainfo_internal(txn.get(), superblock->get(), &old_metainfo);
    update_metainfo(old_metainfo, new_metainfo, txn.get(), superblock.get());

    protocol_reset_data(subregion, btree.get(), txn.get(), superblock.get());
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

        std::vector<char> value(static_cast<const char*>((*i).second.data()),
                                static_cast<const char*>((*i).second.data()) + (*i).second.size());

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
    acquire_superblock_for_read(rwi_read, token, &txn, &superblock, interruptor);

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
    acquire_superblock_for_write(rwi_write, repli_timestamp_t::invalid, 1, token, &txn, &superblock, interruptor);

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
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    assert_thread();
    btree->assert_thread();

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(token);
    wait_interruptible(token->get(), interruptor);

    order_token_t order_token = order_source.check_in("btree_store_t<" + protocol_t::protocol_name + ">::acquire_superblock_for_read").with_read_mode();
    order_token = btree->pre_begin_txn_checkpoint_.check_through(order_token);

    get_btree_superblock_and_txn_for_reading(btree.get(), access, order_token, CACHE_SNAPSHOTTED_NO, sb_out, txn_out);
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

    get_btree_superblock_and_txn(btree.get(), access, expected_change_count, timestamp, order_token, sb_out, txn_out);
}

/* store_view_t interface */
template <class protocol_t>
void btree_store_t<protocol_t>::new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token_out) {
    assert_thread();
    fifo_enforcer_read_token_t token = token_source.enter_read();
    token_out->create(&token_sink, token);
}

template <class protocol_t>
void btree_store_t<protocol_t>::new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token_out) {
    assert_thread();
    fifo_enforcer_write_token_t token = token_source.enter_write();
    token_out->create(&token_sink, token);
}

#include "memcached/protocol.hpp"
template class btree_store_t<memcached_protocol_t>;

#include "rdb_protocol/protocol.hpp"
template class btree_store_t<rdb_protocol_t>;
