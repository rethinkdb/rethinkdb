#ifndef CLUSTERING_REACTOR_REACTOR_JSON_ADAPTERS_HPP_
#define CLUSTERING_REACTOR_REACTOR_JSON_ADAPTERS_HPP_

#include "clustering/reactor/metadata.hpp"
#include "http/json.hpp"
#include "http/json/json_adapter.hpp"

namespace reactor_business_card_details {
// ctx-less json adapter concept for backfill location
json_adapter_if_t::json_adapter_map_t get_json_subfields(backfill_location_t *target);
cJSON *render_as_json(backfill_location_t *target);
void apply_json_to(cJSON *, backfill_location_t *) NORETURN;
void on_subfield_change(backfill_location_t *);

// ctx-less json adapter for primary_when_safe
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(primary_when_safe_t<protocol_t> *);

template <class protocol_t>
cJSON *render_as_json(primary_when_safe_t<protocol_t> *);

template <class protocol_t>
void apply_json_to(cJSON *, primary_when_safe_t<protocol_t> *);

template <class protocol_t>
void on_subfield_change(primary_when_safe_t<protocol_t> *);

// ctx-less json adapter for primary
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(primary_t<protocol_t> *);

template <class protocol_t>
cJSON *render_as_json(primary_t<protocol_t> *);

template <class protocol_t>
void apply_json_to(cJSON *, primary_t<protocol_t> *);

template <class protocol_t>
void on_subfield_change(primary_t<protocol_t> *);


// ctx-less json adapter for secondary_up_to_date
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(secondary_up_to_date_t<protocol_t> *);

template <class protocol_t>
cJSON *render_as_json(secondary_up_to_date_t<protocol_t> *);

template <class protocol_t>
void apply_json_to(cJSON *, secondary_up_to_date_t<protocol_t> *);

template <class protocol_t>
void on_subfield_change(secondary_up_to_date_t<protocol_t> *);


// ctx-less json adapter for secondary_without_primary
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(secondary_without_primary_t<protocol_t> *);

template <class protocol_t>
cJSON *render_as_json(secondary_without_primary_t<protocol_t> *);

template <class protocol_t>
void apply_json_to(cJSON *, secondary_without_primary_t<protocol_t> *);

template <class protocol_t>
void on_subfield_change(secondary_without_primary_t<protocol_t> *);

// ctx-less json adapter for secondary_backfilling
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(secondary_backfilling_t<protocol_t> *);

template <class protocol_t>
cJSON *render_as_json(secondary_backfilling_t<protocol_t> *);

template <class protocol_t>
void apply_json_to(cJSON *, secondary_backfilling_t<protocol_t> *);

template <class protocol_t>
void on_subfield_change(secondary_backfilling_t<protocol_t> *);

// ctx-less json adapter for nothing_when_safe_t
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(nothing_when_safe_t<protocol_t> *);

template <class protocol_t>
cJSON *render_as_json(nothing_when_safe_t<protocol_t> *);

template <class protocol_t>
void apply_json_to(cJSON *, nothing_when_safe_t<protocol_t> *);

template <class protocol_t>
void on_subfield_change(nothing_when_safe_t<protocol_t> *);



// ctx-less json adapter for nothing_t
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(nothing_t<protocol_t> *);

template <class protocol_t>
cJSON *render_as_json(nothing_t<protocol_t> *);

template <class protocol_t>
void apply_json_to(cJSON *, nothing_t<protocol_t> *);

template <class protocol_t>
void on_subfield_change(nothing_t<protocol_t> *);


// ctx-less json adapter for nothing_when_done_erasing
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(nothing_when_done_erasing_t<protocol_t> *);

template <class protocol_t>
cJSON *render_as_json(nothing_when_done_erasing_t<protocol_t> *);

template <class protocol_t>
void apply_json_to(cJSON *, nothing_when_done_erasing_t<protocol_t> *);

template <class protocol_t>
void on_subfield_change(nothing_when_done_erasing_t<protocol_t> *);

} //namespace reactor_business_card_details


//json adapter for reactor_business_card_t
template <class protocol_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(reactor_business_card_t<protocol_t> *);

template <class protocol_t>
cJSON *render_as_json(reactor_business_card_t<protocol_t> *);

template <class protocol_t>
void apply_json_to(cJSON *, reactor_business_card_t<protocol_t> *);

template <class protocol_t>
void on_subfield_change(reactor_business_card_t<protocol_t> *);

#endif /* CLUSTERING_REACTOR_REACTOR_JSON_ADAPTERS_HPP_ */
