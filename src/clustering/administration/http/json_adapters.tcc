// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_JSON_ADAPTERS_TCC_
#define CLUSTERING_ADMINISTRATION_HTTP_JSON_ADAPTERS_TCC_

#include "clustering/administration/http/json_adapters.hpp"

#include <string>
#include <utility>

#include "http/json.hpp"
#include "http/json/json_adapter.hpp"
#include "region/region_json_adapter.hpp"
#include "rpc/semilattice/joins/deletable.hpp"

//json adapter concept for deletable_t
template <class T, class ctx_t>
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(deletable_t<T> *target, const ctx_t &ctx) {
    return with_ctx_get_json_subfields(target->get_mutable(), ctx);
}

template <class T, class ctx_t>
cJSON *with_ctx_render_as_json(deletable_t<T> *target, const ctx_t &ctx) {
    if (target->is_deleted()) {
        return cJSON_CreateNull();
    } else {
        return with_ctx_render_as_json(target->get_mutable(), ctx);
    }
}

template <class T, class ctx_t>
void with_ctx_apply_json_to(cJSON *change, deletable_t<T> *target, const ctx_t &ctx) {
    if (is_null(change)) {
        target->mark_deleted();
    } else if (target->is_deleted()) {
        throw gone_exc_t();
    } else {
        with_ctx_apply_json_to(change, target->get_mutable(), ctx);
    }
}

template <class T, class ctx_t>
void with_ctx_erase_json(deletable_t<T> *target, const ctx_t &) {
    *target = target->get_deletion();
}

template <class T, class ctx_t>
void with_ctx_on_subfield_change(deletable_t<T> *, const ctx_t &) { }

// ctx-less json adapter concept for deletable_t
template <class T>
json_adapter_if_t::json_adapter_map_t get_json_subfields(deletable_t<T> *target) {
    return get_json_subfields(&target->get_mutable());
}

template <class T>
cJSON *render_as_json(deletable_t<T> *target) {
    if (target->is_deleted()) {
        return cJSON_CreateNull();
    } else {
        return render_as_json(&target->get_mutable());
    }
}

template <class T>
void apply_json_to(cJSON *change, deletable_t<T> *target) {
    if (is_null(change)) {
        target->mark_deleted();
    } else if (target->is_deleted()) {
        throw gone_exc_t();
    } else {
        with_ctx_apply_json_to(change, target->get_mutable());
    }
}

template <class T>
void erase_json(deletable_t<T> *target) {
    *target = target->get_deletion();
}


// ctx-less json adapter concept for peer_id_t
// TODO: de-inline these?
inline json_adapter_if_t::json_adapter_map_t get_json_subfields(peer_id_t *) {
    return json_adapter_if_t::json_adapter_map_t();
}

inline cJSON *render_as_json(peer_id_t *target) {
    uuid_u uuid = target->get_uuid();
    return render_as_json(&uuid);
}

inline void apply_json_to(cJSON *change, peer_id_t *target) {
    uuid_u uuid;
    apply_json_to(change, &uuid);
    *target = peer_id_t(uuid);
}




#endif  // CLUSTERING_ADMINISTRATION_HTTP_JSON_ADAPTERS_TCC_
