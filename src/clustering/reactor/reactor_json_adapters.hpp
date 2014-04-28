// Copyright 2010-2013 RethinkDB, all rights reserved.
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

// ctx-less json adapter for primary_when_safe
json_adapter_if_t::json_adapter_map_t get_json_subfields(primary_when_safe_t *);
cJSON *render_as_json(primary_when_safe_t *);
void apply_json_to(cJSON *, primary_when_safe_t *) NORETURN;

// ctx-less json adapter for primary
json_adapter_if_t::json_adapter_map_t get_json_subfields(primary_t *);
cJSON *render_as_json(primary_t *);
void apply_json_to(cJSON *, primary_t *) NORETURN;

// ctx-less json adapter for secondary_up_to_date
json_adapter_if_t::json_adapter_map_t get_json_subfields(secondary_up_to_date_t *);
cJSON *render_as_json(secondary_up_to_date_t *);
void apply_json_to(cJSON *, secondary_up_to_date_t *) NORETURN;

// ctx-less json adapter for secondary_without_primary
json_adapter_if_t::json_adapter_map_t get_json_subfields(secondary_without_primary_t *);
cJSON *render_as_json(secondary_without_primary_t *);
void apply_json_to(cJSON *, secondary_without_primary_t *) NORETURN;

// ctx-less json adapter for secondary_backfilling
json_adapter_if_t::json_adapter_map_t get_json_subfields(secondary_backfilling_t *);
cJSON *render_as_json(secondary_backfilling_t *);
void apply_json_to(cJSON *, secondary_backfilling_t *) NORETURN;

// ctx-less json adapter for nothing_when_safe_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(nothing_when_safe_t *);
cJSON *render_as_json(nothing_when_safe_t *);
void apply_json_to(cJSON *, nothing_when_safe_t *) NORETURN;

// ctx-less json adapter for nothing_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(nothing_t *);
cJSON *render_as_json(nothing_t *);
void apply_json_to(cJSON *, nothing_t *) NORETURN;

// ctx-less json adapter for nothing_when_done_erasing
json_adapter_if_t::json_adapter_map_t get_json_subfields(nothing_when_done_erasing_t *);
cJSON *render_as_json(nothing_when_done_erasing_t *);
void apply_json_to(cJSON *, nothing_when_done_erasing_t *) NORETURN;

} //namespace reactor_business_card_details

// json adapter for reactor_business_card_t::activity_entry_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(reactor_activity_entry_t *target);
cJSON *render_as_json(reactor_activity_entry_t *target);
void apply_json_to(cJSON *change, reactor_activity_entry_t *target);

//json adapter for reactor_business_card_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(reactor_business_card_t *);
cJSON *render_as_json(reactor_business_card_t *);
void apply_json_to(cJSON *, reactor_business_card_t *);




#endif /* CLUSTERING_REACTOR_REACTOR_JSON_ADAPTERS_HPP_ */
