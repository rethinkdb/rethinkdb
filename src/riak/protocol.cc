#include "riak/protocol.hpp"
#include "utils.hpp"

namespace riak {

namespace utils {
/* takes a class C that has an insert method and puts an E in your C */
template <class E>
std::set<E> singleton_set(E e) {
    std::set<E> res;
    res.insert(e);
    return res;
}

template <class E>
std::vector<E> singleton_vector(E e) {
    std::vector<E> res;
    res.push_back(e);
    return res;
}

} //namespace utils 
region_t::region_t(key_spec_t _key_spec) 
    : key_spec(_key_spec)
{ }

bool region_t::contains_functor::operator()(const finite_t & x, const finite_t & y) const {
    return all_in_container_match_predicate(y, boost::bind(&std_contains<finite_t, std::string>, x, _1));
}
bool region_t::contains_functor::operator()(const finite_t &, const hash_range_t & y) const {
    //notice, hash_range_ts are either infinite or empty, thus if y isn't empty
    //it can't possibly be contains in a finite range
    return y.first == y.second;
}

bool region_t::contains_functor::operator()(const hash_range_t & x, const finite_t & y) const {
    return all_in_container_match_predicate(y, boost::bind(&region_t::hash_range_contains_key, x, _1));
}

bool region_t::contains_functor::operator()(const hash_range_t & x, const hash_range_t & y) const {
    return (y.first >= x.first && y.second <= x.second);
}

bool region_t::overlaps_functor::operator()(const finite_t & x, const finite_t & y) const {
    return all_in_container_match_predicate(y, boost::bind(&std_does_not_contain<finite_t, std::string>, x, _1));
}

bool region_t::overlaps_functor::operator()(const finite_t & x, const hash_range_t & y) const {
    return !all_in_container_match_predicate(x, boost::bind(&notf, boost::bind(&region_t::hash_range_contains_key, y, _1)));
}

bool region_t::overlaps_functor::operator()(const hash_range_t & x, const finite_t & y) const {
    return operator()(y, x); //overlaps is commutative
}

bool region_t::overlaps_functor::operator()(const hash_range_t & x, const hash_range_t & y) const {
    return hash_range_contains_hash(x, y.first)  ||
           hash_range_contains_hash(x, y.second) ||
           hash_range_contains_hash(y, x.first);
}

region_t::key_spec_t region_t::intersection_functor::operator()(const finite_t & x, const finite_t & y) const {
    finite_t res;
    for (key_it_t it = x.begin(); it != x.end(); it++) {
        if (std_contains(y, *it)) {
            res.insert(*it);
        }
    }

    return res;
}

region_t::key_spec_t region_t::intersection_functor::operator()(const finite_t & x, const hash_range_t & y) const {
    finite_t res;
    for (key_it_t it = x.begin(); it != x.end(); it++) {
        if (hash_range_contains_key(y, *it)) {
            res.insert(*it);
        }
    }

    return res;
}

region_t::key_spec_t region_t::intersection_functor::operator()(const hash_range_t & x, const finite_t & y) const {
    return operator()(y,x);
}

region_t::key_spec_t region_t::intersection_functor::operator()(const hash_range_t & x, const hash_range_t & y) const {
    if (overlaps_functor()(x,y)) {
        return std::make_pair(std::max(x.first, y.first), std::min(x.second, y.second));
    } else {
        return finite_t();
    }
}

region_t region_t::universe() {
    return region_t(std::make_pair(unsigned(0), ~unsigned(0)));
}

region_t region_t::null() {
    return region_t(std::make_pair(unsigned(0), unsigned(0)));
}

bool region_t::contains(region_t &other) {
    return boost::apply_visitor(contains_functor(), key_spec, other.key_spec);
}

bool region_t::overlaps(region_t &other) {
    overlaps_functor visitor;
    return boost::apply_visitor(visitor, key_spec, other.key_spec);
}

region_t region_t::intersection(region_t &other) {
    return region_t(boost::apply_visitor(intersection_functor(), key_spec, other.key_spec));
}

point_read_t::point_read_t(std::string _key) 
    : key(_key)
{ }

point_read_t::point_read_t(std::string _key, std::pair<int, int> _range)
    : key(_key), range(_range)
{ }

region_t point_read_t::get_region() const {
    region_t::finite_t set;
    set.insert(key);
    return region_t(set);
}

std::vector<read_t> point_read_t::shard(std::vector<region_t> regions) {
    rassert(regions.size() == 1);
    region_t region = get_region();
    rassert(regions[0].overlaps(region));

    std::vector<read_t> res;
    res.push_back(*this);
    return res;
}

region_t bucket_read_t::get_region() const {
    return region_limit;
}

std::vector<read_t> bucket_read_t::shard(std::vector<region_t> regions) {
    std::vector<read_t> res;

    for (std::vector<region_t>::iterator it = regions.begin(); it != regions.end(); it++) {
        res.push_back(read_t(bucket_read_t(*it)));
    }

    return res;
}

region_t mapred_read_t::get_region() const {
    return region;
}

std::vector<read_t> mapred_read_t::shard(std::vector<region_t> regions) {
    std::vector<read_t> res;
    for (std::vector<region_t>::iterator it = regions.begin(); it != regions.end(); it++) {
        res.push_back(read_t(mapred_read_t(it->intersection(region))));
    }
    return res;
}

struct read_get_region_functor : public boost::static_visitor<region_t> {
    region_t operator()(const point_read_t &read) const { return read.get_region(); }
    region_t operator()(const bucket_read_t &read) const { return read.get_region(); }
    region_t operator()(const mapred_read_t &read) const { return read.get_region(); }
};

region_t read_t::get_region() const {
    return boost::apply_visitor(read_get_region_functor(), internal);
}

struct read_shard_functor : public boost::static_visitor<std::vector<read_t> > {
    std::vector<read_t>  operator()(point_read_t read, std::vector<region_t> regions) const { 
        return read.shard(regions); 
    }
    std::vector<read_t>  operator()(bucket_read_t read, std::vector<region_t> regions) const {
        return read.shard(regions); 
    }
    std::vector<read_t>  operator()(mapred_read_t read, std::vector<region_t> regions) const {
        return read.shard(regions); 
    }
};

std::vector<read_t> read_t::shard(std::vector<region_t> regions) {
    boost::variant<std::vector<region_t> > _regions(regions);
    return boost::apply_visitor(read_shard_functor(), internal, _regions);
}

struct read_response_unshard_functor : public boost::static_visitor<read_response_t> {
    read_response_t operator()(const point_read_response_t &, const point_read_response_t &) const { unreachable(); }
    read_response_t operator()(const point_read_response_t &, const bucket_read_response_t &) const { unreachable(); }
    read_response_t operator()(const point_read_response_t &, const mapred_read_response_t &) const { unreachable(); }
    read_response_t operator()(const bucket_read_response_t &, const point_read_response_t &) const { unreachable(); }
    read_response_t operator()(const bucket_read_response_t &x, const bucket_read_response_t &y) const { return bucket_read_response_t (x, y); }
    read_response_t operator()(const bucket_read_response_t &, const mapred_read_response_t &) const { unreachable(); }
    read_response_t operator()(const mapred_read_response_t &, const point_read_response_t &) const { unreachable(); }
    read_response_t operator()(const mapred_read_response_t &, const bucket_read_response_t &) const { unreachable(); }
    read_response_t operator()(const mapred_read_response_t &, const mapred_read_response_t &) const { crash("Not implemented"); }
};

read_response_t read_t::unshard(std::vector<read_response_t> responses) {
    rassert(!responses.empty());
    while (responses.size() != 1) {
        //notice if copy constructors for these read responses are expensive this could become kind of slow
        read_response_t combined = boost::apply_visitor(read_response_unshard_functor(), *(responses.rend()), *(responses.rend() + 1));

        responses.pop_back();
        responses.pop_back();

        responses.push_back(combined);
    }

    return responses.front();
}

mapred_read_response_t mapred_read_response_t::unshard(std::vector<mapred_read_response_t>, unshard_ctx_t *) {
    crash("Not implemented");
}

read_response_t read_enactor_vistor_t::operator()(point_read_t read, riak_interface_t *riak) {
    if (read.range) {
        return read_response_t(point_read_response_t(riak->get_object(read.key, *(read.range))));
    } else {
        return read_response_t(point_read_response_t(riak->get_object(read.key)));
    }
}

read_response_t read_enactor_vistor_t::operator()(bucket_read_t, riak_interface_t *riak) {
    return read_response_t(bucket_read_response_t(riak->objects()));
}

read_response_t read_enactor_vistor_t::operator()(mapred_read_t, riak_interface_t *) {
    crash("Not implemented");
}

// implementation of writes
// set write
region_t set_write_t::get_region() {
    return region_t(utils::singleton_set<std::string>(object.key));
}

std::vector<write_t> set_write_t::shard(std::vector<region_t> regions) {
    rassert(regions.size() == 1);
    return utils::singleton_vector<write_t>(write_t(*this));
}

region_t delete_write_t::get_region() {
    return region_t(utils::singleton_set<std::string>(key));
}

std::vector<write_t> delete_write_t::shard(std::vector<region_t> regions) {
    rassert(regions.size() == 1);
    return utils::singleton_vector<write_t>(write_t(*this));
}

struct write_get_region_functor : public boost::static_visitor<region_t> {
    region_t operator()(set_write_t w) const { return w.get_region(); }
    region_t operator()(delete_write_t w) const { return w.get_region(); }
};

region_t write_t::get_region() {
    return boost::apply_visitor(write_get_region_functor(), internal);
}

struct write_shard_functor : public boost::static_visitor<std::vector<write_t> > {
    std::vector<write_t> operator()(set_write_t w, std::vector<region_t> regions) const {
        return w.shard(regions);
    }
    std::vector<write_t> operator()(delete_write_t w, std::vector<region_t> regions) const {
        return w.shard(regions);
    }
};

std::vector<write_t> write_t::shard(std::vector<region_t> regions) {
    boost::variant<std::vector<region_t> > regions_variant(regions);
    return boost::apply_visitor(write_shard_functor(), internal, regions_variant);
}

store_t::store_t(region_t _region, riak_interface_t *_interface)
    : region(_region), interface(_interface), backfilling(false)
{ }

region_t store_t::get_region() {
    return region;
}

bool store_t::is_coherent() {
    return !backfilling;
}

repli_timestamp_t store_t::get_timestamp() {
    crash("Not implemented");
}

read_response_t store_t::read(read_t , order_token_t ) {
    crash("Not implemented");
}

write_response_t store_t::write(write_t, repli_timestamp_t, order_token_t) {
    crash("Not implemented");
}

/* bool store_t::is_backfilling() {
    return backfilling;
}

void backfillee_chunk(backfill_chunk_t);

void backfillee_end(backfill_end_t);

void backfillee_cancel(); */

} //namespace riak 
