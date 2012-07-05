#include "errors.hpp"
#include <boost/bind.hpp>

#include "btree/erase_range.hpp"
#include "btree/slice.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/vector_stream.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/protocol.hpp"
#include "serializer/config.hpp"

typedef rdb_protocol_details::backfill_atom_t backfill_atom_t;

typedef rdb_protocol_t::region_t region_t;

typedef rdb_protocol_t::read_t read_t;
typedef rdb_protocol_t::read_response_t read_response_t;

typedef rdb_protocol_t::point_read_t point_read_t;
typedef rdb_protocol_t::point_read_response_t point_read_response_t;

typedef rdb_protocol_t::write_t write_t;
typedef rdb_protocol_t::write_response_t write_response_t;

typedef rdb_protocol_t::point_write_t point_write_t;
typedef rdb_protocol_t::point_write_response_t point_write_response_t;

typedef rdb_protocol_t::point_delete_t point_delete_t;
typedef rdb_protocol_t::point_delete_response_t point_delete_response_t;

typedef rdb_protocol_t::backfill_chunk_t backfill_chunk_t;

typedef rdb_protocol_t::backfill_progress_t backfill_progress_t;


/* read_t::get_region implementation */
struct r_get_region_visitor : public boost::static_visitor<key_range_t> {
    key_range_t operator()(const point_read_t &pr) const {
        return key_range_t(key_range_t::closed, pr.key, key_range_t::closed, pr.key);
    }
};

key_range_t read_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(r_get_region_visitor(), read);
}

/* read_t::shard implementation */

struct r_shard_visitor : public boost::static_visitor<read_t> {
    explicit r_shard_visitor(const key_range_t &_key_range)
        : key_range(_key_range)
    { }

    read_t operator()(const point_read_t &pr) const {
        rassert(key_range_t(key_range_t::closed, pr.key, key_range_t::closed, pr.key) == key_range);
        return read_t(pr);
    }
    const key_range_t &key_range;
};

read_t read_t::shard(const key_range_t &region) const THROWS_NOTHING {
    return boost::apply_visitor(r_shard_visitor(region), read);
}

/* read_t::unshard implementation */

read_response_t rdb_protocol_t::read_t::unshard(std::vector<read_response_t> responses, temporary_cache_t *) const THROWS_NOTHING {
    rassert(responses.size() == 1);
    return responses[0];
}

read_response_t rdb_protocol_t::read_t::multistore_unshard(const std::vector<read_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING {
    return unshard(responses, cache);
}


/* write_t::get_region() implementation */

struct w_get_region_visitor : public boost::static_visitor<key_range_t> {
    key_range_t operator()(const point_write_t &pw) const {
        return key_range_t(key_range_t::closed, pw.key, key_range_t::closed, pw.key);
    }
    
    key_range_t operator()(const point_delete_t &pd) const {
        return key_range_t(key_range_t::closed, pd.key, key_range_t::closed, pd.key);
    }
};

key_range_t write_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(w_get_region_visitor(), write);
}

/* write_t::shard implementation */

struct w_shard_visitor : public boost::static_visitor<write_t> {
    explicit w_shard_visitor(const key_range_t &_key_range)
        : key_range(_key_range)
    { }

    write_t operator()(const point_write_t &pw) const {
        rassert(key_range_t(key_range_t::closed, pw.key, key_range_t::closed, pw.key) == key_range);
        return write_t(pw);
    }
    write_t operator()(const point_delete_t &pd) const {
        rassert(key_range_t(key_range_t::closed, pd.key, key_range_t::closed, pd.key) == key_range);
        return write_t(pd);
    }
    const key_range_t &key_range;
};

write_t write_t::shard(key_range_t region) const THROWS_NOTHING {
    return boost::apply_visitor(w_shard_visitor(region), write);
}

write_response_t write_t::unshard(std::vector<write_response_t> responses, temporary_cache_t *) const THROWS_NOTHING {
    rassert(responses.size() == 1);
    return responses[0];
}

write_response_t write_t::multistore_unshard(const std::vector<write_response_t>& responses, temporary_cache_t *cache) const THROWS_NOTHING {
    return unshard(responses, cache);
}

typedef rdb_protocol_t::store_t store_t;
typedef store_t::metainfo_t metainfo_t;

store_t::store_t(const std::string& filename, bool create, perfmon_collection_t *_perfmon_collection)
    : store_view_t<rdb_protocol_t>(key_range_t::universe()),
      perfmon_collection(_perfmon_collection)
{
    if (create) {
        standard_serializer_t::create(
            standard_serializer_t::dynamic_config_t(),
            standard_serializer_t::private_dynamic_config_t(filename),
            standard_serializer_t::static_config_t(),
            perfmon_collection
            );
    }

    serializer.reset(new standard_serializer_t(
        standard_serializer_t::dynamic_config_t(),
        standard_serializer_t::private_dynamic_config_t(filename),
        perfmon_collection
        ));

    if (create) {
        mirrored_cache_static_config_t cache_static_config;
        cache_t::create(serializer.get(), &cache_static_config);
    }

    cache_dynamic_config.max_size = GIGABYTE;
    cache_dynamic_config.max_dirty_size = GIGABYTE / 2;
    cache.reset(new cache_t(serializer.get(), &cache_dynamic_config, perfmon_collection));

    if (create) {
        btree_slice_t::create(cache.get());
    }

    btree.reset(new btree_slice_t(cache.get(), perfmon_collection));

    if (create) {
        // Initialize metainfo to an empty `binary_blob_t` because its domain is
        // required to be `key_range_t::universe()` at all times
        /* Wow, this is a lot of lines of code for a simple concept. Can we do better? */
        boost::scoped_ptr<real_superblock_t> superblock;
        boost::scoped_ptr<transaction_t> txn;
        order_token_t order_token = order_source.check_in("rdb_protocol_t::store_t::store_t");
        order_token = btree->order_checkpoint_.check_through(order_token);
        get_btree_superblock(btree.get(), rwi_write, 1, repli_timestamp_t::invalid, order_token, &superblock, txn);
        buf_lock_t* sb_buf = superblock->get();
        clear_superblock_metainfo(txn.get(), sb_buf);

        vector_stream_t key;
        write_message_t msg;
        key_range_t kr = key_range_t::universe();   // `operator<<` needs a non-const reference
        msg << kr;
        DEBUG_ONLY_VAR int res = send_write_message(&key, &msg);
        rassert(!res);
        set_superblock_metainfo(txn.get(), sb_buf, key.vector(), std::vector<char>());
    }
}

store_t::~store_t() { }

/* store_view_t interface */
void store_t::new_read_token(boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token_out) {
    fifo_enforcer_read_token_t token = token_source.enter_read();
    token_out.reset(new fifo_enforcer_sink_t::exit_read_t(&token_sink, token));
}

void store_t::new_write_token(boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token_out) {
    fifo_enforcer_write_token_t token = token_source.enter_write();
    token_out.reset(new fifo_enforcer_sink_t::exit_write_t(&token_sink, token));
}

void store_t::acquire_superblock_for_read(
        access_t access,
        bool snapshot,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
        boost::scoped_ptr<transaction_t> &txn_out,
        boost::scoped_ptr<real_superblock_t> &sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    btree->assert_thread();

    boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> local_token;
    local_token.swap(token);
    wait_interruptible(local_token.get(), interruptor);

    order_token_t order_token = order_source.check_in("rdb_protocol_t::store_t::acquire_superblock_for_read");
    order_token = btree->order_checkpoint_.check_through(order_token);

    get_btree_superblock_for_reading(btree.get(), access, order_token, snapshot, &sb_out, txn_out);
}

void store_t::acquire_superblock_for_backfill(
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
        boost::scoped_ptr<transaction_t> &txn_out,
        boost::scoped_ptr<real_superblock_t> &sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    btree->assert_thread();

    boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> local_token;
    local_token.swap(token);
    wait_interruptible(local_token.get(), interruptor);

    order_token_t order_token = order_source.check_in("rdb_protocol_t::store_t::acquire_superblock_for_backfill");
    order_token = btree->order_checkpoint_.check_through(order_token);

    get_btree_superblock_for_backfilling(btree.get(), order_token, &sb_out, txn_out);
}

void store_t::acquire_superblock_for_write(
        access_t access,
        int expected_change_count,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
        boost::scoped_ptr<transaction_t> &txn_out,
        boost::scoped_ptr<real_superblock_t> &sb_out,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    btree->assert_thread();

    boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> local_token;
    local_token.swap(token);
    wait_interruptible(local_token.get(), interruptor);

    order_token_t order_token = order_source.check_in("rdb_protocol_t::store_t::acquire_superblock_for_write");
    order_token = btree->order_checkpoint_.check_through(order_token);

    get_btree_superblock(btree.get(), access, expected_change_count, repli_timestamp_t::invalid, order_token, &sb_out, txn_out);
}

metainfo_t store_t::get_metainfo(
        UNUSED order_token_t order_token,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;
    acquire_superblock_for_read(rwi_read, false, token, txn, superblock, interruptor);

    return get_metainfo_internal(txn.get(), superblock->get());
}

region_map_t<rdb_protocol_t, binary_blob_t> store_t::get_metainfo_internal(transaction_t *txn, buf_lock_t *sb_buf) const THROWS_NOTHING {
    std::vector<std::pair<std::vector<char>, std::vector<char> > > kv_pairs;
    get_superblock_metainfo(txn, sb_buf, kv_pairs);   // FIXME: this is inefficient, cut out the middleman (vector)

    std::vector<std::pair<region_t, binary_blob_t> > result;
    for (std::vector<std::pair<std::vector<char>, std::vector<char> > >::iterator i = kv_pairs.begin(); i != kv_pairs.end(); ++i) {
        const std::vector<char> &value = i->second;

        region_t region;
        {
            vector_read_stream_t key(&i->first);
            DEBUG_ONLY_VAR archive_result_t res = deserialize(&key, &region);
            rassert(!res);
        }

        result.push_back(std::make_pair(
            region,
            binary_blob_t(value.begin(), value.end())
        ));
    }
    region_map_t<rdb_protocol_t, binary_blob_t> res(result.begin(), result.end());
    rassert(res.get_domain() == key_range_t::universe());
    return res;
}

void store_t::set_metainfo(
        const metainfo_t &new_metainfo,
        UNUSED order_token_t order_token,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;
    acquire_superblock_for_write(rwi_write, 1, token, txn, superblock, interruptor);

    region_map_t<rdb_protocol_t, binary_blob_t> old_metainfo = get_metainfo_internal(txn.get(), superblock->get());
    update_metainfo(old_metainfo, new_metainfo, txn.get(), superblock.get());
}

struct read_visitor_t : public boost::static_visitor<read_response_t> {
    read_response_t operator()(const point_read_t& get) {
        return read_response_t(
            rdb_get(get.key, btree, txn.get(), superblock.get()));
    }

    read_visitor_t(btree_slice_t *btree_, boost::scoped_ptr<transaction_t>& txn_, boost::scoped_ptr<superblock_t> &superblock_) :
        btree(btree_), txn(txn_), superblock(superblock_) { }

private:
    btree_slice_t *btree;
    boost::scoped_ptr<transaction_t>& txn;
    boost::scoped_ptr<superblock_t> &superblock;
};

read_response_t store_t::read(
        DEBUG_ONLY(const metainfo_checker_t<rdb_protocol_t>& metainfo_checker, )
        const rdb_protocol_t::read_t &read,
        UNUSED order_token_t order_token,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;
    acquire_superblock_for_read(rwi_read, false, token, txn, superblock, interruptor);

    check_metainfo(DEBUG_ONLY(metainfo_checker, ) txn.get(), superblock.get());

    /* Ugly hack */
    boost::scoped_ptr<superblock_t> superblock2;
    superblock.swap(*reinterpret_cast<boost::scoped_ptr<real_superblock_t> *>(&superblock2));

    read_visitor_t v(btree.get(), txn, superblock2);
    return boost::apply_visitor(v, read.read);
}

struct write_visitor_t : public boost::static_visitor<write_response_t> {
    write_response_t operator()(const point_write_t &w) {
        return write_response_t(
            rdb_set(w.key, w.data, btree, timestamp, txn.get(), superblock));
    }

    write_response_t operator()(const point_delete_t &d) {
        return write_response_t(
            rdb_delete(d.key, btree, timestamp, txn.get(), superblock));
    }

    write_visitor_t(btree_slice_t *btree_, boost::scoped_ptr<transaction_t>& txn_, superblock_t *superblock_, repli_timestamp_t timestamp_) : btree(btree_), txn(txn_), superblock(superblock_), timestamp(timestamp_) { }

private:
    btree_slice_t *btree;
    boost::scoped_ptr<transaction_t>& txn;
    superblock_t *superblock;
    repli_timestamp_t timestamp;
};

write_response_t store_t::write(
        DEBUG_ONLY(const metainfo_checker_t<rdb_protocol_t>& metainfo_checker, )
        const metainfo_t& new_metainfo,
        const rdb_protocol_t::write_t &write,
        transition_timestamp_t timestamp,
        UNUSED order_token_t order_token,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;
    const int expected_change_count = 2; // FIXME: this is incorrect, but will do for now
    acquire_superblock_for_write(rwi_write, expected_change_count, token, txn, superblock, interruptor);

    check_and_update_metainfo(DEBUG_ONLY(metainfo_checker, ) new_metainfo, txn.get(), superblock.get());

    write_visitor_t v(btree.get(), txn, superblock.get(), timestamp.to_repli_timestamp());
    return boost::apply_visitor(v, write.write);
}

struct rdb_backfill_callback_t : public backfill_callback_t {
    typedef rdb_protocol_t::backfill_chunk_t chunk_t;
    const boost::function<void(chunk_t)> &chunk_fun;

    explicit rdb_backfill_callback_t(const boost::function<void(chunk_t)> &chunk_fun_) : chunk_fun(chunk_fun_) { }

    void on_delete_range(const key_range_t &range) {
        chunk_fun(chunk_t::delete_range(range));
    }

    void on_deletion(const btree_key_t *key, UNUSED repli_timestamp_t recency) {
        chunk_fun(chunk_t::delete_key(to_store_key(key), recency));
    }

    void on_keyvalue(const backfill_atom_t& atom) {
        chunk_fun(chunk_t::set_key(atom));
    }
    ~rdb_backfill_callback_t() { }
protected:
    store_key_t to_store_key(const btree_key_t *key) {
        return store_key_t(key->size, key->contents);
    }
};

class refcount_superblock_t : public superblock_t {
public:
    refcount_superblock_t(superblock_t *sb, int rc) :
        sub_superblock(sb), refcount(rc) { }

    void release() {
        refcount--;
        rassert(refcount >= 0);
        if (refcount == 0) {
            sub_superblock->release();
            sub_superblock = NULL;
        }
    }

    block_id_t get_root_block_id() const {
        return sub_superblock->get_root_block_id();
    }

    void set_root_block_id(const block_id_t new_root_block) {
        sub_superblock->set_root_block_id(new_root_block);
    }

    block_id_t get_stat_block_id() const {
        return sub_superblock->get_stat_block_id();
    }

    void set_stat_block_id(block_id_t new_stat_block) {
        sub_superblock->set_stat_block_id(new_stat_block);
    }

    void set_eviction_priority(eviction_priority_t eviction_priority) {
        sub_superblock->set_eviction_priority(eviction_priority);
    }

    eviction_priority_t get_eviction_priority() {
        return sub_superblock->get_eviction_priority();
    }

private:
    superblock_t *sub_superblock;
    int refcount;
};

static void call_rdb_backfill(int i, btree_slice_t *btree, const std::vector<std::pair<region_t, state_timestamp_t> > &regions,
        backfill_callback_t *callback, transaction_t *txn, superblock_t *superblock, backfill_progress_t *progress) {
    traversal_progress_t *p = new traversal_progress_t;
    progress->add_constituent(p);
    repli_timestamp_t timestamp = regions[i].second.to_repli_timestamp();
    rdb_backfill(btree, regions[i].first, timestamp, callback, txn, superblock, p);
}

bool store_t::send_backfill(
        const region_map_t<rdb_protocol_t, state_timestamp_t> &start_point,
        const boost::function<bool(const metainfo_t&)> &should_backfill,
        const boost::function<void(backfill_chunk_t)> &chunk_fun,
        backfill_progress_t *progress,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;
    acquire_superblock_for_backfill(token, txn, superblock, interruptor);

    metainfo_t metainfo = get_metainfo_internal(txn.get(), superblock->get()).mask(start_point.get_domain());
    if (should_backfill(metainfo)) {
        rdb_backfill_callback_t callback(chunk_fun);

        std::vector<std::pair<region_t, state_timestamp_t> > regions(start_point.begin(), start_point.end());
        refcount_superblock_t refcount_wrapper(superblock.get(), regions.size());
        pmap(regions.size(), boost::bind(&call_rdb_backfill, _1,
            btree.get(), regions, &callback, txn.get(), &refcount_wrapper, progress));

        return true;
    } else {
        return false;
    }
}

struct receive_backfill_visitor_t : public boost::static_visitor<> {
    receive_backfill_visitor_t(btree_slice_t *btree_, transaction_t *txn_, superblock_t *superblock_, signal_t *interruptor_) : btree(btree_), txn(txn_), superblock(superblock_), interruptor(interruptor_) { }
    void operator()(const backfill_chunk_t::delete_key_t& delete_key) const {
        // FIXME: we ignored delete_key.recency here. Should we use it in place of repli_timestamp_t::invalid?
        rdb_delete(delete_key.key, btree, repli_timestamp_t::invalid, txn, superblock);
    }
    void operator()(const backfill_chunk_t::delete_range_t& delete_range) const {
        range_key_tester_t tester(delete_range.range);
        rdb_erase_range(btree, &tester, delete_range.range, txn, superblock);
    }
    void operator()(const backfill_chunk_t::key_value_pair_t& kv) const {
        const backfill_atom_t& bf_atom = kv.backfill_atom;
        rdb_set(bf_atom.key, bf_atom.value,
                btree, repli_timestamp_t::invalid,
                txn, superblock);
    }
private:
    /* TODO: This might be redundant. I thought that `key_tester_t` was only
    originally necessary because in v1.1.x the hashing scheme might be different
    between the source and destination machines. */
    struct range_key_tester_t : public key_tester_t {
        explicit range_key_tester_t(const key_range_t& delete_range_) : delete_range(delete_range_) { }
        bool key_should_be_erased(const btree_key_t *key) {
            return delete_range.contains_key(key->contents, key->size);
        }

        const key_range_t& delete_range;
    };
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    signal_t *interruptor;  // FIXME: interruptors are not used in btree code, so this one ignored.
};

void store_t::receive_backfill(
        const backfill_chunk_t &chunk,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;
    const int expected_change_count = 1; // FIXME: this is probably not correct

    acquire_superblock_for_write(rwi_write, expected_change_count, token, txn, superblock, interruptor);

    boost::apply_visitor(receive_backfill_visitor_t(btree.get(), txn.get(), superblock.get(), interruptor), chunk.val);
}

void store_t::reset_data(
        const region_t &subregion,
        const metainfo_t &new_metainfo,
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> &token,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {

    boost::scoped_ptr<real_superblock_t> superblock;
    boost::scoped_ptr<transaction_t> txn;

    // We're passing 2 for the expected_change_count based on the
    // reasoning that we're probably going to touch a leaf-node-sized
    // range of keys and that it won't be aligned right on a leaf node
    // boundary.
    // TODO that's not reasonable; reset_data() is sometimes used to wipe out
    // entire databases.
    const int expected_change_count = 2;
    acquire_superblock_for_write(rwi_write, expected_change_count, token, txn, superblock, interruptor);

    region_map_t<rdb_protocol_t, binary_blob_t> old_metainfo = get_metainfo_internal(txn.get(), superblock->get());
    update_metainfo(old_metainfo, new_metainfo, txn.get(), superblock.get());

    always_true_key_tester_t key_tester;
    rdb_erase_range(btree.get(), &key_tester, subregion, txn.get(), superblock.get());
}

void store_t::check_and_update_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<rdb_protocol_t>& metainfo_checker, )
        const metainfo_t &new_metainfo,
        transaction_t *txn,
        real_superblock_t *superblock) const
        THROWS_NOTHING {
    metainfo_t old_metainfo = check_metainfo(DEBUG_ONLY(metainfo_checker, ) txn, superblock);
    update_metainfo(old_metainfo, new_metainfo, txn, superblock);
}

store_t::metainfo_t store_t::check_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<rdb_protocol_t>& metainfo_checker, )
        transaction_t *txn,
        real_superblock_t *superblock) const
        THROWS_NOTHING {

    region_map_t<rdb_protocol_t, binary_blob_t> old_metainfo = get_metainfo_internal(txn, superblock->get());
#ifndef NDEBUG
    metainfo_checker.check_metainfo(old_metainfo.mask(metainfo_checker.get_domain()));
#endif
    return old_metainfo;
}

void rdb_protocol_t::store_t::update_metainfo(const metainfo_t &old_metainfo, const metainfo_t &new_metainfo, transaction_t *txn, real_superblock_t *superblock) const THROWS_NOTHING {
    region_map_t<rdb_protocol_t, binary_blob_t> updated_metadata = old_metainfo;
    updated_metadata.update(new_metainfo);

    rassert(updated_metadata.get_domain() == key_range_t::universe());

    buf_lock_t* sb_buf = superblock->get();
    clear_superblock_metainfo(txn, sb_buf);

    for (region_map_t<rdb_protocol_t, binary_blob_t>::const_iterator i = updated_metadata.begin(); i != updated_metadata.end(); ++i) {

        vector_stream_t key;
        write_message_t msg;
        msg << i->first;
        DEBUG_ONLY_VAR int res = send_write_message(&key, &msg);
        rassert(!res);

        std::vector<char> value(static_cast<const char*>((*i).second.data()),
                                static_cast<const char*>((*i).second.data()) + (*i).second.size());
        set_superblock_metainfo(txn, sb_buf, key.vector(), value); // FIXME: this is not efficient either, see how value is created
    }
}

key_range_t rdb_protocol_t::cpu_sharding_subspace(int subregion_number, UNUSED int num_cpu_shards) {
    rassert(subregion_number >= 0);
    rassert(subregion_number < num_cpu_shards);

    if (subregion_number == 0) {
        return key_range_t::universe();
    }
    return key_range_t::empty();
}
