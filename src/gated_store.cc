#include "gated_store.hpp"

// gated_get_store_t

gated_get_store_t::gated_get_store_t(get_store_t *internal) : internal(internal) { }

get_result_t gated_get_store_t::get(const store_key_t &key) {
    if (gate.is_open()) {
        threadsafe_gate_t::entry_t entry(&gate);
        return internal->get(key);
    } else {
        get_result_t r;
        r.is_not_allowed = true;
        return r;
    }
}

rget_result_t gated_get_store_t::rget(rget_bound_mode_t left_mode, const store_key_t &left_key,
        rget_bound_mode_t right_mode, const store_key_t &right_key) {
    if (gate.is_open()) {
        threadsafe_gate_t::entry_t entry(&gate);
        return internal->rget(left_mode, left_key, right_mode, right_key);
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

mutation_result_t gated_set_store_interface_t::change(const mutation_t &mut) {
    if (gate.is_open()) {
        threadsafe_gate_t::entry_t entry(&gate);
        return internal->change(mut);
    } else {
        return boost::apply_visitor(not_allowed_visitor_t(), mut.mutation);
    }
}
