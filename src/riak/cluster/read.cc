#include "riak/cluster/read.hpp"
#include "riak/cluster/std_utils.hpp"

namespace riak {
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

std::vector<read_t> point_read_t::shard(DEBUG_ONLY_VAR std::vector<region_t> regions) {
    rassert(regions.size() == 1);
    region_t region = get_region();
    rassert(regions[0].overlaps(region));

    return utils::singleton_vector(read_t(*this));
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

mapred_read_response_t mapred_read_response_t::unshard(std::vector<mapred_read_response_t>, temporary_cache *) {
    crash("Not implemented");
}

} //namespace riak
