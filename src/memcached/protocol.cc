#include "errors.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include "containers/vector_stream.hpp"

#include "memcached/protocol.hpp"
#include "memcached/queries.hpp"
#include "btree/erase_range.hpp"
#include "containers/iterators.hpp"
#include "btree/slice.hpp"
#include "btree/operations.hpp"


/* `memcached_protocol_t::read_t::get_region()` */

static key_range_t::bound_t convert_bound_mode(rget_bound_mode_t rbm) {
    switch (rbm) {
        case rget_bound_open: return key_range_t::open;
        case rget_bound_closed: return key_range_t::closed;
        case rget_bound_none: return key_range_t::none;
        default: unreachable();
    }
}

struct read_get_region_visitor_t : public boost::static_visitor<key_range_t> {
    key_range_t operator()(get_query_t get) {
        return key_range_t(key_range_t::closed, get.key, key_range_t::closed, get.key);
    }
    key_range_t operator()(rget_query_t rget) {
        return key_range_t(
            convert_bound_mode(rget.left_mode),
            rget.left_key,
            convert_bound_mode(rget.right_mode),
            rget.right_key
            );
    }
};

key_range_t memcached_protocol_t::read_t::get_region() const THROWS_NOTHING {
    read_get_region_visitor_t v;
    return boost::apply_visitor(v, query);
}

/* `memcached_protocol_t::read_t::shard()` */

struct read_shard_visitor_t : public boost::static_visitor<memcached_protocol_t::read_t> {
    explicit read_shard_visitor_t(const key_range_t &r) : region(r) { }
    const key_range_t &region;
    memcached_protocol_t::read_t operator()(get_query_t get) {
        rassert(region == key_range_t(key_range_t::closed, get.key, key_range_t::closed, get.key));
        return memcached_protocol_t::read_t(get);
    }
    memcached_protocol_t::read_t operator()(UNUSED rget_query_t original_rget) {
        rassert(region_is_superset(
            key_range_t(
                convert_bound_mode(original_rget.left_mode),
                original_rget.left_key,
                convert_bound_mode(original_rget.right_mode),
                original_rget.right_key
                ),
            region
            ));
        rget_query_t sub_rget;
        sub_rget.left_mode = rget_bound_closed;
        sub_rget.left_key = region.left;
        if (region.right.unbounded) {
            sub_rget.right_mode = rget_bound_none;
        } else {
            sub_rget.right_mode = rget_bound_open;
            sub_rget.right_key = region.right.key;
        }
        return memcached_protocol_t::read_t(sub_rget);
    }
};

memcached_protocol_t::read_t memcached_protocol_t::read_t::shard(const key_range_t &r) const THROWS_NOTHING {
    read_shard_visitor_t v(r);
    return boost::apply_visitor(v, query);
}

/* `memcached_protocol_t::read_t::unshard()` */

typedef merge_ordered_data_iterator_t<key_with_data_buffer_t, key_with_data_buffer_t::less> merged_results_iterator_t;

struct read_unshard_visitor_t : public boost::static_visitor<memcached_protocol_t::read_response_t> {
    std::vector<memcached_protocol_t::read_response_t> &bits;

    explicit read_unshard_visitor_t(std::vector<memcached_protocol_t::read_response_t> &b) : bits(b) { }
    memcached_protocol_t::read_response_t operator()(UNUSED get_query_t get) {
        rassert(bits.size() == 1);
        return memcached_protocol_t::read_response_t(boost::get<get_result_t>(bits[0].result));
    }
    memcached_protocol_t::read_response_t operator()(UNUSED rget_query_t rget) {
        boost::shared_ptr<merged_results_iterator_t> merge_iterator(new merged_results_iterator_t());
        for (int i = 0; i < (int)bits.size(); i++) {
            merge_iterator->add_mergee(boost::get<rget_result_t>(bits[i].result));
        }
        return memcached_protocol_t::read_response_t(rget_result_t(merge_iterator));
    }
};

memcached_protocol_t::read_response_t memcached_protocol_t::read_t::unshard(std::vector<read_response_t> responses, UNUSED temporary_cache_t *cache) const THROWS_NOTHING {
    read_unshard_visitor_t v(responses);
    return boost::apply_visitor(v, query);
}

/* `memcached_protocol_t::write_t::get_region()` */

struct write_get_region_visitor_t : public boost::static_visitor<key_range_t> {
    /* All the types of mutation have a member called `key` */
    template<class mutation_t>
    key_range_t operator()(mutation_t mut) {
        return key_range_t(key_range_t::closed, mut.key, key_range_t::closed, mut.key);
    }
};

key_range_t memcached_protocol_t::write_t::get_region() const THROWS_NOTHING {
    write_get_region_visitor_t v;
    return apply_visitor(v, mutation);
}

/* `memcached_protocol_t::write_t::shard()` */

memcached_protocol_t::write_t memcached_protocol_t::write_t::shard(UNUSED key_range_t region) const THROWS_NOTHING {
    rassert(region == get_region());
    return *this;
}

/* `memcached_protocol_t::write_response_t::unshard()` */

memcached_protocol_t::write_response_t memcached_protocol_t::write_t::unshard(std::vector<memcached_protocol_t::write_response_t> responses, UNUSED temporary_cache_t *cache) const THROWS_NOTHING {
    /* TODO: Make sure the request type matches the response type */
    rassert(responses.size() == 1);
    return responses[0];
}

/* `dummy_memcached_store_view_t::dummy_memcached_store_view_t()` */

dummy_memcached_store_view_t::dummy_memcached_store_view_t(key_range_t region, btree_slice_t *b) :
    store_view_t<memcached_protocol_t>(region), btree(b) { }

boost::shared_ptr<store_view_t<memcached_protocol_t>::read_transaction_t> dummy_memcached_store_view_t::begin_read_transaction(UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    crash("stub");
}

boost::shared_ptr<store_view_t<memcached_protocol_t>::write_transaction_t> dummy_memcached_store_view_t::begin_write_transaction(UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    crash("stub");
}

namespace arc = boost::archive;

memcached_store_view_t::txn_t::txn_t(btree_slice_t *btree_, order_source_t &order_source, bool read_txn) : btree(btree_) {
    btree->assert_thread();

    access_t access = read_txn ? rwi_read : rwi_write;

    token = order_source.check_in("memcached_store_view_t::txn_t");
    token = btree->order_checkpoint_.check_through(token);
    if (is_read_mode(access)) {
        get_btree_superblock_for_reading(btree, access, token, false, &superblock, txn); // FIXME: for rgets we really want to pass snapshotting=true
    } else {
        const int expected_change_count = 0;    // FIXME: this is not correct, but should do for now. See issue #501.
        get_btree_superblock(btree, access, expected_change_count, repli_timestamp_t::invalid, token, &superblock, txn);
    }
}

buf_t * memcached_store_view_t::txn_t::get_superblock_buf() {
    return static_cast<real_superblock_t*>(superblock.sb.get())->get()->buf(); // FIXME: boy, this is ugly! I told you, Sam!
}

region_map_t<memcached_protocol_t,binary_blob_t> memcached_store_view_t::txn_t::get_metadata(UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    std::vector<std::pair<std::vector<char>,std::vector<char> > > kv_pairs;
    get_superblock_metainfo(txn.get(), get_superblock_buf(), kv_pairs);   // FIXME: this is inefficient, cut out the middleman (vector)

    std::vector<std::pair<memcached_protocol_t::region_t,binary_blob_t> > result;
    for (std::vector<std::pair<std::vector<char>,std::vector<char> > >::iterator i = kv_pairs.begin(); i != kv_pairs.end(); ++i) {
        vector_streambuf_t<> key((*i).first);
        const std::vector<char> &value = (*i).second;

        memcached_protocol_t::region_t region;
        {
            arc::binary_iarchive region_archive(key, arc::no_header);
            region_archive >> region;
        }

        result.push_back(std::make_pair(
            region,
            binary_blob_t(value.begin(), value.end())
        ));
    }
    return region_map_t<memcached_protocol_t,binary_blob_t>(result);
}

void memcached_store_view_t::txn_t::set_metadata(UNUSED const region_map_t<memcached_protocol_t,binary_blob_t> &new_metadata) THROWS_NOTHING {
    // FIXME: This particular method of updating metainfo is not very efficient
    cond_t interruptor;
    region_map_t<memcached_protocol_t,binary_blob_t> updated_metadata = get_metadata(&interruptor).update(new_metadata);
    clear_superblock_metainfo(txn.get(), get_superblock_buf());

    for (region_map_t<memcached_protocol_t,binary_blob_t>::const_iterator i = updated_metadata.begin(); i != updated_metadata.end(); ++i) {
        vector_streambuf_t<> key;
        {
            arc::binary_oarchive key_archive(key, arc::no_header);
            key_archive << (*i).first;
        }

        std::vector<char> value(static_cast<const char*>((*i).second.data()), static_cast<const char*>((*i).second.data()) + (*i).second.size());
        set_superblock_metainfo(txn.get(), get_superblock_buf(), key.vector(), value); // FIXME: this is not efficient either, see how value is created
    }
}

struct btree_operation_visitor_t : public boost::static_visitor<memcached_protocol_t::read_response_t> {
    btree_operation_visitor_t(btree_slice_t *btree_, order_token_t& token_, boost::scoped_ptr<transaction_t>& txn_, got_superblock_t& superblock_) : btree(btree_), token(token_), txn(txn_), superblock(superblock_) { }

    memcached_protocol_t::read_response_t operator()(const get_query_t& get) {
        return memcached_protocol_t::read_response_t(btree->get(get.key, token, txn.get(), superblock));
    }
    memcached_protocol_t::read_response_t operator()(const rget_query_t& rget) {
        return memcached_protocol_t::read_response_t(btree->rget(rget.left_mode, rget.left_key, rget.right_mode, rget.right_key, token, txn, superblock));
    }

private:
    btree_slice_t *btree;
    order_token_t& token;
    boost::scoped_ptr<transaction_t>& txn;
    got_superblock_t& superblock;
};

memcached_protocol_t::read_response_t memcached_store_view_t::txn_t::read(const memcached_protocol_t::read_t &read, UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    btree_operation_visitor_t v(btree, token, txn, superblock);
    return boost::apply_visitor(v, read.query);
}

memcached_protocol_t::write_response_t memcached_store_view_t::txn_t::write(const memcached_protocol_t::write_t &write, transition_timestamp_t timestamp) THROWS_NOTHING {
    castime_t cas = castime_t(write.proposed_cas, timestamp.to_repli_timestamp());
    return memcached_protocol_t::write_response_t(btree->change(write.mutation, cas, token, txn.get(), superblock).result);
}

struct memcached_backfill_callback_t : public backfill_callback_t {
    typedef memcached_protocol_t::backfill_chunk_t chunk_t;
    const boost::function<void(chunk_t)> &chunk_fun;

    memcached_backfill_callback_t(const boost::function<void(chunk_t)> &chunk_fun_) : chunk_fun(chunk_fun_) { }

    void on_delete_range(const btree_key_t *left_exclusive, const btree_key_t *right_inclusive) {
        chunk_fun(chunk_t::delete_range(
            key_range_t(
                left_exclusive ? key_range_t::open : key_range_t::none, left_exclusive ? store_key_t(left_exclusive->size, left_exclusive->contents) : store_key_t(),
                right_inclusive ? key_range_t::closed : key_range_t::none, right_inclusive ? store_key_t(right_inclusive->size, right_inclusive->contents) : store_key_t()
            )
        ));
    }

    void on_deletion(const btree_key_t *key, UNUSED repli_timestamp_t recency) {
        chunk_fun(chunk_t::delete_key(to_store_key(key), recency));
    }

    void on_keyvalue(const backfill_atom_t& atom) {
        chunk_fun(chunk_t::set_key(atom));
    }
    ~memcached_backfill_callback_t() { }
protected:
    store_key_t to_store_key(const btree_key_t *key) {
        return store_key_t(key->size, key->contents);
    }
};

void memcached_store_view_t::txn_t::send_backfill(const region_map_t<memcached_protocol_t,state_timestamp_t> &start_point, const boost::function<void(memcached_protocol_t::backfill_chunk_t)> &chunk_fun, UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    memcached_backfill_callback_t callback(chunk_fun);

    for (region_map_t<memcached_protocol_t,state_timestamp_t>::const_iterator i = start_point.begin(); i != start_point.end(); i++) {
        const memcached_protocol_t::region_t& range = (*i).first;
        repli_timestamp_t since_when = (*i).second.to_repli_timestamp();    // FIXME: this loses precision
        btree->backfill(static_cast<const key_range_t&>(range), since_when, &callback, token);
    }
}

struct receive_backfill_visitor_t : public boost::static_visitor<> {
    receive_backfill_visitor_t(btree_slice_t *btree_, order_token_t& token_, signal_t *interruptor_) : btree(btree_), token(token_), interruptor(interruptor_) { }
    void operator()(const memcached_protocol_t::backfill_chunk_t::delete_key_t& delete_key) const {
        // FIXME: we ignored delete_key.recency here
        btree->change(mutation_t(delete_mutation_t(delete_key.key, true)), castime_t(), token);
    }
    void operator()(const memcached_protocol_t::backfill_chunk_t::delete_range_t& delete_range) const {
        const key_range_t& range = delete_range.range;
        range_key_tester_t tester(range);
        bool left_supplied = range.left.size > 0;
        bool right_supplied = !range.right.unbounded;
        btree->backfill_delete_range(&tester, left_supplied, range.left, right_supplied, range.right.key, token);
    }
    void operator()(const memcached_protocol_t::backfill_chunk_t::key_value_pair_t& kv) const {
        const backfill_atom_t& bf_atom = kv.backfill_atom;
        btree->change(mutation_t(sarc_mutation_t(bf_atom.key, bf_atom.value, bf_atom.flags, bf_atom.exptime, add_policy_yes, replace_policy_yes, bf_atom.cas_or_zero)), castime_t(), token);
    }
private:
    struct range_key_tester_t : public key_tester_t {
        range_key_tester_t(const key_range_t& delete_range_) : delete_range(delete_range_) { }
        bool key_should_be_erased(const btree_key_t *key) {
            return delete_range.contains_key(key->contents, key->size);
        }

        const key_range_t& delete_range;
    };
    btree_slice_t *btree;
    order_token_t& token;
    signal_t *interruptor;  // FIXME: interruptors are not used in btree code, so this one ignored.
};

void memcached_store_view_t::txn_t::receive_backfill(const memcached_protocol_t::backfill_chunk_t &chunk, UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    boost::apply_visitor(receive_backfill_visitor_t(btree, token, interruptor), chunk.val);
}

