// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_JSON_ADAPTERS_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_JSON_ADAPTERS_HPP_

#include <string>

#include "http/json/json_adapter.hpp"
#include "rpc/connectivity/peer_id.hpp"
#include "rpc/semilattice/joins/deletable.hpp"

/* There are a couple of semilattice structures that we need to have json
 * adaptable for the administration server, this code shouldn't go with the
 * definition of these structures because they're deep in the rpc directory
 * which generally doesn't concern itself with how people interact with its
 * structures. */

//json adapter concept for deletable_t
template <class T, class ctx_t>
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(deletable_t<T> *, const ctx_t &);

template <class T, class ctx_t>
cJSON *with_ctx_render_as_json(deletable_t<T> *, const ctx_t &);

template <class T, class ctx_t>
void with_ctx_apply_json_to(cJSON *, deletable_t<T> *, const ctx_t &);

template <class T, class ctx_t>
void with_ctx_erase_json(deletable_t<T> *, const ctx_t &);

template <class T, class ctx_t>
void with_ctx_on_subfield_change(deletable_t<T> *, const ctx_t &);

// ctx-less json adapter concept for deletable_t
template <class T>
json_adapter_if_t::json_adapter_map_t get_json_subfields(deletable_t<T> *);

template <class T>
cJSON *render_as_json(deletable_t<T> *);

template <class T>
void apply_json_to(cJSON *, deletable_t<T> *);

template <class T>
void erase_json(deletable_t<T> *);

// ctx-less json adapter concept for peer_id_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(peer_id_t *);

cJSON *render_as_json(peer_id_t *);

void apply_json_to(cJSON *, peer_id_t *);


#include "clustering/administration/http/json_adapters.tcc"

#endif /* CLUSTERING_ADMINISTRATION_HTTP_JSON_ADAPTERS_HPP_ */
