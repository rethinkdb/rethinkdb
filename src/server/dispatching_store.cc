#include "server/dispatching_store.hpp"
#include "btree/slice.hpp"
#include "btree/erase_range.hpp"   /* for `key_tester_t` */
#include "server/key_value_store.hpp"

dispatching_store_t::dispatching_store_t(btree_slice_t *ss) : substore(ss) { }

void dispatching_store_t::set_dispatcher(boost::function<mutation_t(const mutation_t &, castime_t, order_token_t)> disp) {
    assert_thread();
    dispatcher = disp;
}

mutation_result_t dispatching_store_t::change(const mutation_t &m, castime_t castime, order_token_t token) {
    assert_thread();
    if (dispatcher) {
        sink.check_out(token);
        order_token_t tmp_token = pre_dispatch_source.check_in(token.tag() + "+dp_token");
        order_token_t dispatchee_token = order_source_for_dispatchee.check_in(token.tag() + "+dispatchee");
        const mutation_t& m2 = dispatcher(m, castime, dispatchee_token);
        post_dispatch_sink.check_out(tmp_token);
        order_token_t substore_token = substore_order_source.check_in(token.tag() + "+dispatching_store_t::change");

        return substore->change(m2, castime, substore_token);
    } else {
        // TODO: What if we go from having a dispatcher to having no dispatcher?
        order_token_t substore_token = substore_order_source.check_in(token.tag() + "+dispatching_store_t::change");
        return substore->change(m, castime, substore_token);
    }
}

get_result_t dispatching_store_t::get(const store_key_t &key, order_token_t token) {
    sink.check_out(token);
    order_token_t substore_token = substore_order_source.check_in(token.tag() + "+dispatching_store_t::get").with_read_mode();
    return substore->get(key, substore_token);
}

rget_result_t dispatching_store_t::rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token) {
    sink.check_out(token);
    order_token_t substore_token = substore_order_source.check_in(token.tag() + "+dispatching_store_t::rget").with_read_mode();
    return substore->rget(left_mode, left_key, right_mode, right_key, substore_token);
}

class hash_key_tester_t : public key_tester_t {
public:
    hash_key_tester_t(int hash_value, int hashmod) : hash_value_(hash_value), hashmod_(hashmod) {
        rassert(hashmod > 0);
        rassert(hash_value >= 0 && hash_value < hashmod);
    }

    bool key_should_be_erased(const btree_key_t *key) {
        return int(btree_key_value_store_t::hash(key->contents, key->size) % hashmod_) == hash_value_;
    }

private:
    int hash_value_, hashmod_;
};

void dispatching_store_t::backfill_delete_range(int hash_value, int hashmod,
                                                bool left_key_supplied, const store_key_t& left_key_exclusive,
                                                bool right_key_supplied, const store_key_t& right_key_inclusive,
                                                order_token_t token) {
    sink.check_out(token);
    order_token_t substore_token = substore_order_source.check_in(token.tag() + "+dispatching_store_t::backfill_delete_range");
    hash_key_tester_t tester(hash_value, hashmod);
    substore->backfill_delete_range(&tester,
                                    left_key_supplied, left_key_exclusive,
                                    right_key_supplied, right_key_inclusive,
                                    substore_token);
}

void dispatching_store_t::set_replication_clock(repli_timestamp_t t, order_token_t token) {
    sink.check_out(token);
    substore->set_replication_clock(t, substore_order_source.check_in(token.tag() + "shard_store_t::set_replication_clock"));
}


