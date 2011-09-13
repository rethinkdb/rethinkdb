#include "memcached/protocol.hpp"
#include "containers/iterators.hpp"

/* Comparison operators for `key_range_t::right_bound_t` are declared in here
because nothing else ever needs them. */

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
                *this = key_range_t();
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

bool key_range_t::contains(key_range_t range) const {
    if (range.left < left) return false;
    if (range.right > right) return false;
    return true;
}

bool key_range_t::overlaps(key_range_t range) const {
    return (range.left < right && left < range.right);
}

key_range_t key_range_t::intersection(key_range_t range) const {
    if (!overlaps(range)) return key_range_t();
    key_range_t ixn;
    ixn.left = left < range.left ? range.left : left;
    ixn.right = right > range.right ? range.right : right;
    return ixn;
}

bool key_range_t::covered_by(std::vector<key_range_t> ranges) const {
    right_bound_t cursor = left;
    while (cursor < right) {
        bool moved = false;
        for (std::vector<key_range_t>::iterator it = ranges.begin(); it != ranges.end(); it++) {
            if (cursor >= (*it).left && cursor < (*it).right) {
                cursor = (*it).right;
                moved = true;
                break;
            }
        }
        if (!moved) return false;
    }
    return true;
}

bool operator==(key_range_t a, key_range_t b) {
    return a.left == b.left && a.right == b.right;
}

bool operator!=(key_range_t a, key_range_t b) {
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

key_range_t memcached_protocol_t::read_t::get_region() const {
    read_get_region_visitor_t v;
    return boost::apply_visitor(v, query);
}

/* `memcached_protocol_t::read_t::shard()` */

struct read_shard_visitor_t : public boost::static_visitor<std::vector<memcached_protocol_t::read_t> > {
    explicit read_shard_visitor_t(std::vector<key_range_t> &r) : regions(r) { }
    std::vector<key_range_t> &regions;
    std::vector<memcached_protocol_t::read_t> operator()(get_query_t get) {
        rassert(regions.size() == 1);
        rassert(regions[0].contains(key_range_t(key_range_t::closed, get.key, key_range_t::closed, get.key)));
        std::vector<memcached_protocol_t::read_t> vec;
        vec.push_back(get);
        return vec;
    }
    std::vector<memcached_protocol_t::read_t> operator()(UNUSED rget_query_t original_rget) {
        std::vector<memcached_protocol_t::read_t> subreads;
        for (int i = 0; i < (int)regions.size(); i++) {
            rassert(regions[i].overlaps(
                key_range_t(
                    convert_bound_mode(original_rget.left_mode),
                    original_rget.left_key,
                    convert_bound_mode(original_rget.right_mode),
                    original_rget.right_key
                    )));
            rget_query_t sub_rget;
            sub_rget.left_mode = rget_bound_closed;
            sub_rget.left_key = regions[i].left;
            if (regions[i].right.unbounded) {
                sub_rget.right_mode = rget_bound_none;
            } else {
                sub_rget.right_mode = rget_bound_open;
                sub_rget.right_key = regions[i].right.key;
            }
            subreads.push_back(sub_rget);
        }
        return subreads;
    }
};

std::vector<memcached_protocol_t::read_t> memcached_protocol_t::read_t::shard(std::vector<key_range_t> regions) const {
    read_shard_visitor_t v(regions);
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

memcached_protocol_t::read_response_t memcached_protocol_t::read_t::unshard(std::vector<read_response_t> responses, UNUSED temporary_cache_t *cache) const {
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

key_range_t memcached_protocol_t::write_t::get_region() const {
    write_get_region_visitor_t v;
    return apply_visitor(v, mutation);
}

/* `memcached_protocol_t::write_t::shard()` */

std::vector<memcached_protocol_t::write_t> memcached_protocol_t::write_t::shard(UNUSED std::vector<key_range_t> regions) const {
    rassert(regions.size() == 1);
    std::vector<memcached_protocol_t::write_t> vec;
    vec.push_back(*this);
    return vec;
}

/* `memcached_protocol_t::write_response_t::unshard()` */

memcached_protocol_t::write_response_t memcached_protocol_t::write_t::unshard(std::vector<memcached_protocol_t::write_response_t> responses, UNUSED temporary_cache_t *cache) const {
    /* TODO: Make sure the request type matches the response type */
    rassert(responses.size() == 1);
    return responses[0];
}

/* `memcached_protocol_t::store_t::create()` */

void memcached_protocol_t::store_t::create(serializer_t *ser, key_range_t) {
    mirrored_cache_static_config_t cache_static_config;
    cache_t::create(ser, &cache_static_config);
    mirrored_cache_config_t cache_dynamic_config;
    cache_t cache(ser, &cache_dynamic_config);
    btree_slice_t::create(&cache);
}

/* `memcached_protocol_t::store_t::store_t()` */

memcached_protocol_t::store_t::store_t(serializer_t *ser, key_range_t r) :
    cache(ser, &cache_config),
    btree(&cache),
    region(r)
    { }

/* `memcached_protocol_t::store_t::get_region()` */

key_range_t memcached_protocol_t::store_t::get_region() {
    return region;
}

/* `memcached_protocol_t::store_t::read()` */

struct read_perform_visitor_t : public boost::static_visitor<memcached_protocol_t::read_response_t> {
    read_perform_visitor_t(btree_slice_t *_btree, order_token_t _tok) : btree(_btree), tok(_tok) { }
    btree_slice_t *btree;
    order_token_t tok;
    memcached_protocol_t::read_response_t operator()(get_query_t get) {
        return btree->get(get.key, tok);
    }
    memcached_protocol_t::read_response_t operator()(rget_query_t rget) {
        return btree->rget(
            rget.left_mode,
            rget.left_key,
            rget.right_mode,
            rget.right_key,
            tok
            );
    }
};

memcached_protocol_t::read_response_t memcached_protocol_t::store_t::read(memcached_protocol_t::read_t read, order_token_t tok, UNUSED signal_t *interruptor) {
    read_perform_visitor_t v(&btree, tok);
    return boost::apply_visitor(v, read.query);
}

/* `memcached_protocol_t::store_t::write()` */

/* The return type of `set_store_t::change()` is `mutation_result_t`, which is
equivalent but not identical to `memcached_protocol_t::write_response_t`. This
converts between them.

TODO: `mutation_result_t` should go away. */
struct convert_response_visitor_t : public boost::static_visitor<memcached_protocol_t::write_response_t> {
    template<class result_t>
    memcached_protocol_t::write_response_t operator()(result_t res) const {
        return res;
    }
};

struct write_perform_visitor_t : public boost::static_visitor<memcached_protocol_t::write_response_t> {
    write_perform_visitor_t(btree_slice_t *_btree, castime_t _castime, order_token_t _tok)
        : btree(_btree), castime(_castime), tok(_tok) { }
    btree_slice_t *btree;
    castime_t castime;
    order_token_t tok;
    /* All mutation types can be converted to a `set_store_t`-style
    `mutation_t`. */
    template<class mutation_t>
    memcached_protocol_t::write_response_t operator()(mutation_t mut) {
        mutation_result_t res = btree->change(mut, castime, tok);
        return boost::apply_visitor(convert_response_visitor_t(), res.result);
    }
};

memcached_protocol_t::write_response_t memcached_protocol_t::store_t::write(memcached_protocol_t::write_t write, UNUSED transition_timestamp_t timestamp, order_token_t tok, UNUSED signal_t *interruptor) {
    // TODO: Hook up timestamp
    write_perform_visitor_t v(&btree, castime_t(write.proposed_cas, repli_timestamp_t::invalid), tok);
    return boost::apply_visitor(v, write.mutation);
}
