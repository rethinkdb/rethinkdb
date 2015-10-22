// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/query_params.hpp"

#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/optargs.hpp"
#include "rdb_protocol/pseudo_time.hpp"
#include "rdb_protocol/query_cache.hpp"

namespace ql {

query_params_t::query_id_t::query_id_t(query_params_t::query_id_t &&other) :
        intrusive_list_node_t(std::move(other)),
        parent(other.parent),
        value_(other.value_) {
    parent->assert_thread();
    other.parent = nullptr;
}

query_params_t::query_id_t::query_id_t(query_cache_t *_parent) :
        parent(_parent),
        value_(parent->next_query_id++) {
    // Guarantee correct ordering.
    query_id_t *last_newest = parent->outstanding_query_ids.tail();
    guarantee(last_newest == nullptr || last_newest->value() < value_);
    guarantee(value_ >= parent->oldest_outstanding_query_id.get());

    parent->outstanding_query_ids.push_back(this);
}

query_params_t::query_id_t::~query_id_t() {
    if (parent != nullptr) {
        parent->assert_thread();
    } else {
        rassert(!in_a_list());
    }

    if (in_a_list()) {
        parent->outstanding_query_ids.remove(this);
        if (value_ == parent->oldest_outstanding_query_id.get()) {
            query_id_t *next_outstanding_id = parent->outstanding_query_ids.head();
            if (next_outstanding_id == nullptr) {
                parent->oldest_outstanding_query_id.set_value(parent->next_query_id);
            } else {
                guarantee(next_outstanding_id->value() > value_);
                parent->oldest_outstanding_query_id.set_value(
                    next_outstanding_id->value());
            }
        }
    }
}

uint64_t query_params_t::query_id_t::value() const {
    guarantee(in_a_list());
    return value_;
}

// If the query wants a reply, we can release the query id, which is only used for
// tracking the ordering of noreply queries for the purpose of noreply_wait.
void query_params_t::maybe_release_query_id() {
    if (!noreply) {
        query_id_t destroyer(std::move(id));
    }
}

query_params_t::query_params_t(int64_t _token,
                               ql::query_cache_t *_query_cache,
                               scoped_ptr_t<term_storage_t> &&_term_storage) :
        query_cache(_query_cache),
        term_storage(std::move(_term_storage)),
        id(query_cache), token(_token), noreply(false), profile(false) {
    // Parse out information that is needed before query evaluation
    type = term_storage->query_type();
    noreply = term_storage->static_optarg_as_bool("noreply", noreply);
    profile = term_storage->static_optarg_as_bool("profile", profile);
}

} // namespace ql
