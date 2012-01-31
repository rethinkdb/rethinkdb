#include "riak/cluster/protocol.hpp"
#include "utils.hpp"

namespace riak {

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

struct read_functor : public boost::static_visitor<read_response_t> {
    read_response_t operator()(point_read_t read, riak_interface_t *riak) const {
        if (read.range) {
            return read_response_t(point_read_response_t(riak->get_object(read.key, *(read.range))));
        } else {
            return read_response_t(point_read_response_t(riak->get_object(read.key)));
        }
    }

    read_response_t operator()(bucket_read_t, riak_interface_t *riak) const {
        return read_response_t(bucket_read_response_t(riak->objects()));
    }

    read_response_t operator()(mapred_read_t, riak_interface_t *) const {
        crash("Not implemented");
    }
};

read_response_t store_t::read(read_t read, order_token_t) {
    boost::variant<riak_interface_t *> interface_variant(interface);
    return boost::apply_visitor(read_functor(),  read.internal, interface_variant);
}

struct write_functor : public boost::static_visitor<write_response_t> {
    write_response_t operator()(set_write_t write, riak_interface_t *riak) const {
        set_write_response_t res;

        riak_interface_t::set_result_t set_result = riak->store_object(write.object);

        res.result = set_result;
        if (set_result == riak_interface_t::CREATED) {
            res.key_if_created = write.object.key;
        }

        if (write.return_body) {
            res.object_if_requested = write.object;
        }

        return res;
    }
    write_response_t operator()(delete_write_t write, riak_interface_t *riak) const {
        return delete_write_response_t(riak->delete_object(write.key));
    }
};

write_response_t store_t::write(write_t write, repli_timestamp_t, order_token_t) {
    boost::variant<riak_interface_t *> interface_variant(interface);
    return boost::apply_visitor(write_functor(), write.internal, interface_variant);
}

bool store_t::is_backfilling() {
    return backfilling;
}

void store_t::backfillee_chunk(backfill_chunk_t) {
    crash("Not implemented");
}

void store_t::backfillee_end(backfill_end_t) {
    crash("Not implemented");
}

void store_t::backfillee_cancel() {
    crash("Not implemented");
}

} //namespace riak 
