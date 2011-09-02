#include "riak/cluster/protocol.hpp"
#include "utils.hpp"

namespace riak {

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
