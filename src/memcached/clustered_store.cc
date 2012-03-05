#include "memcached/clustered_store.hpp"

cluster_get_store_t::cluster_get_store_t(namespace_interface_t<memcached_protocol_t>& _namespace_if) : namespace_if(_namespace_if) { }

get_result_t cluster_get_store_t::get(const store_key_t& key, UNUSED sequence_group_t *seq_group, order_token_t token) {
    memcached_protocol_t::read_t r((get_query_t(key)));
    cond_t cond;    // TODO: we should pass a real cond to this method, to allow request interruption
    memcached_protocol_t::read_response_t res(namespace_if.read(r, token, &cond));
    rassert(boost::get<get_result_t>(&res.result) != NULL);
    return boost::get<get_result_t>(res.result);
}

rget_result_t cluster_get_store_t::rget(UNUSED sequence_group_t *seq_group, rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token) {
    memcached_protocol_t::read_t r(rget_query_t(left_mode, left_key, right_mode, right_key));
    cond_t cond;    // TODO: we should pass a real cond to this method, to allow request interruption
    memcached_protocol_t::read_response_t res(namespace_if.read(r, token, &cond));
    rassert(boost::get<rget_result_t>(&res.result) != NULL);
    return boost::get<rget_result_t>(res.result);
}

cluster_get_store_t::~cluster_get_store_t() { }

cluster_set_store_t::cluster_set_store_t(namespace_interface_t<memcached_protocol_t>& _namespace_if) : namespace_if(_namespace_if) { }

mutation_result_t cluster_set_store_t::change(UNUSED sequence_group_t *seq_group, const mutation_t& m, castime_t cas, order_token_t token) {
    memcached_protocol_t::write_t w(m, cas.proposed_cas);   // FIXME: we ignored the timestamp here, is it okay?
    cond_t cond;
    memcached_protocol_t::write_response_t res(namespace_if.write(w, token, &cond));
    return mutation_result_t(res.result);
}

cluster_set_store_t::~cluster_set_store_t() { }

