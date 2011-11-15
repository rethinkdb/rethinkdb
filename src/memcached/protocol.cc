#include "memcached/protocol.hpp"
#include "containers/iterators.hpp"

/* Comparison operators for `key_range_t::right_bound_t` are declared in here
because nothing outside of this file ever needs them. */

bool operator==(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    return a.unbounded == b.unbounded && a.key == b.key;
}
bool operator!=(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    return !(a == b);
}
bool operator<(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    if (a.unbounded) return false;
    if (b.unbounded) return true;
    return a.key < b.key;
}
bool operator<=(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    return a == b || a < b;
}
bool operator>(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    return b < a;
}
bool operator>=(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    return b <= a;
}

key_range_t::key_range_t() :
    left(""), right(store_key_t("")) { }

key_range_t::key_range_t(bound_t lm, store_key_t l, bound_t rm, store_key_t r) {
    switch (lm) {
        case closed:
            left = l;
            break;
        case open:
            left = l;
            if (left.increment()) {
                break;
            } else {
                /* Our left bound is the largest possible key, and we are open
                on the left-hand side. So we are empty. */
                *this = key_range_t::empty();
                return;
            }
        case none:
            left = store_key_t("");
            break;
        default:
            unreachable();
    }

    switch (rm) {
        case closed:
            if (r.increment()) {
                right = right_bound_t(r);
                break;
            } else {
                /* Our right bound is the largest possible key, and we are
                closed on the right-hand side. The only way to express this is
                to set `right` to `right_bound_t()`. */
                right = right_bound_t();
                break;
            }
        case open:
            right = right_bound_t(r);
            break;
        case none:
            right = right_bound_t();
            break;
        default:
            unreachable();
    }
}

bool region_is_superset(const key_range_t &potential_superset, const key_range_t &potential_subset) THROWS_NOTHING {

    /* Special-case empty ranges */
    if (potential_subset.left == potential_subset.right) return true;

    if (potential_superset.left > potential_subset.left) return false;
    if (potential_superset.right < potential_subset.right) return false;

    return true;
}

key_range_t region_intersection(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING {
    if (!region_overlaps(r1, r2)) {
        return key_range_t::empty();
    }
    key_range_t ixn;
    ixn.left = r1.left < r2.left ? r2.left : r1.left;
    ixn.right = r1.right > r2.right ? r2.right : r1.right;
    return ixn;
}

static bool compare_range_by_left(const key_range_t &r1, const key_range_t &r2) {
    return r1.left < r2.left;
}

key_range_t region_join(const std::vector<key_range_t> &vec) THROWS_ONLY(bad_join_exc_t, bad_region_exc_t) {
    if (vec.empty()) {
        return key_range_t::empty();
    } else {
        std::vector<key_range_t> sorted = vec;
        std::sort(sorted.begin(), sorted.end(), &compare_range_by_left);
        key_range_t::right_bound_t cursor = sorted[0].left;
        for (int i = 0; i < (int)sorted.size(); i++) {
            if (cursor < sorted[i].left) {
                /* There's a gap between this region and the region on its left,
                so their union isn't a `key_range_t`. */
                throw bad_region_exc_t();
            } else if (cursor > sorted[i].left) {
                /* This region overlaps with the region on its left, so the join
                is bad. */
                throw bad_join_exc_t();
            } else {
                /* The regions match exactly; move on to the next one. */
                cursor = sorted[i].right;
            }
        }
        key_range_t union_;
        union_.left = sorted[0].left;
        union_.right = cursor;
        return union_;
    }
}

bool region_overlaps(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING {
    return (r1.left < r2.right && r2.left < r1.right && !region_is_empty(r1) && !region_is_empty(r2));
}

bool operator==(key_range_t a, key_range_t b) THROWS_NOTHING {
    return a.left == b.left && a.right == b.right;
}

bool operator!=(key_range_t a, key_range_t b) THROWS_NOTHING {
    return !(a == b);
}

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
        return get;
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
        return sub_rget;
    }
};

memcached_protocol_t::read_t memcached_protocol_t::read_t::shard(const key_range_t &r) const THROWS_NOTHING {
    read_shard_visitor_t v(r);
    return boost::apply_visitor(v, query);
}

/* `memcached_protocol_t::read_t::unshard()` */

typedef merge_ordered_data_iterator_t<key_with_data_buffer_t, key_with_data_buffer_t::less> merged_results_iterator_t;

struct read_unshard_visitor_t : public boost::static_visitor<memcached_protocol_t::read_response_t> {
    explicit read_unshard_visitor_t(std::vector<memcached_protocol_t::read_response_t> &b) : bits(b) { }
    std::vector<memcached_protocol_t::read_response_t> &bits;
    memcached_protocol_t::read_response_t operator()(UNUSED get_query_t get) {
        rassert(bits.size() == 1);
        return boost::get<get_result_t>(bits[0].result);
    }
    memcached_protocol_t::read_response_t operator()(UNUSED rget_query_t rget) {
        boost::shared_ptr<merged_results_iterator_t> merge_iterator(new merged_results_iterator_t());
        for (int i = 0; i < (int)bits.size(); i++) {
            merge_iterator->add_mergee(boost::get<rget_result_t>(bits[i].result));
        }
        return merge_iterator;
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

