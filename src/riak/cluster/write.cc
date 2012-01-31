#include "riak/cluster/write.hpp"
#include "riak/cluster/std_utils.hpp"

namespace riak {

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

struct write_response_unshard_functor : public boost::static_visitor<write_response_t> {
    write_response_t operator()(set_write_response_t, set_write_response_t) const { unreachable(); }
    write_response_t operator()(set_write_response_t, delete_write_response_t) const { unreachable(); }
    write_response_t operator()(delete_write_response_t, set_write_response_t) const { unreachable(); }
    write_response_t operator()(delete_write_response_t, delete_write_response_t) const { unreachable(); }
};

write_response_t unshard(std::vector<write_response_t> responses, temporary_cache_t *) {
    rassert(!responses.empty());
    while (responses.size() != 1) {
        //notice if copy constructors for these read responses are expensive this could become kind of slow
        write_response_t combined = boost::apply_visitor(write_response_unshard_functor(), *(responses.rend()), *(responses.rend() + 1));

        responses.pop_back();
        responses.pop_back();

        responses.push_back(combined);
    }

    return responses.front();
}

} //namespace riak 
