#include "gated_store.hpp"

// gated_get_store_t

gated_get_store_t::gated_get_store_t(get_store_t *internal) : internal(internal) { }

get_result_t gated_get_store_t::get(const store_key_t &key, sequence_group_t *seq_group, order_token_t token) {
    if (gate.is_open()) {
        threadsafe_gate_t::entry_t entry(&gate);
        return internal->get(key, seq_group, token);
    } else {
        get_result_t r;
        r.is_not_allowed = true;
        return r;
    }
}

rget_result_t gated_get_store_t::rget(sequence_group_t *seq_group, rget_bound_mode_t left_mode, const store_key_t &left_key,
                                      rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token) {
    if (gate.is_open()) {
        threadsafe_gate_t::entry_t entry(&gate);
        return internal->rget(seq_group, left_mode, left_key, right_mode, right_key, token);
    } else {
        return rget_result_t(NULL);
    }
}

// gated_set_store_interface_t

gated_set_store_interface_t::gated_set_store_interface_t(set_store_interface_t *internal) :
    internal(internal) { }

// This visitor constructs a "not allowed" response of the correct type
struct not_allowed_visitor_t : public boost::static_visitor<mutation_result_t> {
    mutation_result_t operator()(UNUSED const get_cas_mutation_t& m) const {
        get_result_t r;
        r.is_not_allowed = true;
        return r;
    }
    mutation_result_t operator()(UNUSED const sarc_mutation_t& m) const {
        return sr_not_allowed;
    }
    mutation_result_t operator()(UNUSED const incr_decr_mutation_t& m) const {
        return incr_decr_result_t(incr_decr_result_t::idr_not_allowed);
    }
    mutation_result_t operator()(UNUSED const append_prepend_mutation_t& m) const {
        return apr_not_allowed;
    }
    mutation_result_t operator()(UNUSED const delete_mutation_t& m) const {
        return dr_not_allowed;
    }
};

mutation_result_t gated_set_store_interface_t::change(sequence_group_t *seq_group, const mutation_t &mut, order_token_t token) {
    if (gate.is_open()) {
        threadsafe_gate_t::entry_t entry(&gate);
        return internal->change(seq_group, mut, token);
    } else {
        return boost::apply_visitor(not_allowed_visitor_t(), mut.mutation);
    }
}
