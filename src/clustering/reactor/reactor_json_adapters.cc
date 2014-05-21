// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/reactor/reactor_json_adapters.hpp"

#include <string>
#include <vector>

#include "http/json.hpp"
#include "clustering/administration/http/json_adapters.hpp"
#include "region/region_json_adapter.hpp"
#include "rdb_protocol/protocol.hpp"

namespace reactor_business_card_details {

// ctx-less json adapter concept for backfill location
json_adapter_if_t::json_adapter_map_t get_json_subfields(backfill_location_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["backfill_session_id"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<backfill_session_id_t>(&target->backfill_session_id));
    res["peer_id"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<peer_id_t>(&target->peer_id));
    res["activity_id"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<reactor_activity_id_t>(&target->activity_id));
    return res;
}

cJSON *render_as_json(backfill_location_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *, backfill_location_t *) {
    throw permission_denied_exc_t("Can't write to backfill_location_t objects.\n");
}


// ctx-less json adapter for primary_when_safe
json_adapter_if_t::json_adapter_map_t get_json_subfields(primary_when_safe_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["type"] = boost::shared_ptr<json_adapter_if_t>(new json_temporary_adapter_t<std::string>("primary_when_safe"));
    res["backfillers"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<std::vector<backfill_location_t> >(&target->backfills_waited_on));
    return res;
}

cJSON *render_as_json(primary_when_safe_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *, primary_when_safe_t *) {
    throw permission_denied_exc_t("Can't write to primary_when_safe_t objects.\n");
}


// ctx-less json adapter for primary
json_adapter_if_t::json_adapter_map_t get_json_subfields(primary_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["type"] = boost::shared_ptr<json_adapter_if_t>(new json_temporary_adapter_t<std::string>("primary"));
    res["replier_present"] = boost::shared_ptr<json_adapter_if_t>(new json_temporary_adapter_t<bool>(target->replier.is_initialized()));
    // TODO: git blame this and ask the person why the code is commented out.
    //res["broadcaster"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<broadcaster_business_card_t>(&target->broadcaster));
    //res["replier"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<boost::optional<replier_business_card_t> >(&target->replier));
    return res;
}

cJSON *render_as_json(primary_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *, primary_t *) {
    throw permission_denied_exc_t("Can't write to primary_when_safe_t objects.\n");
}


// ctx-less json adapter for secondary_up_to_date
json_adapter_if_t::json_adapter_map_t get_json_subfields(secondary_up_to_date_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["type"] = boost::shared_ptr<json_adapter_if_t>(new json_temporary_adapter_t<std::string>("secondary_up_to_date"));
    res["branch_id"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<branch_id_t>(&target->branch_id));
    // TODO: git blame this and ask why it's commented out.
    //res["replier"] =   boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<replier_business_card_t>(&target->replier));
    return res;
}

cJSON *render_as_json(secondary_up_to_date_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *, secondary_up_to_date_t *) {
    throw permission_denied_exc_t("Can't write to primary_safe_t objects.\n");
}


// ctx-less json adapter for secondary_without_primary
json_adapter_if_t::json_adapter_map_t get_json_subfields(secondary_without_primary_t *) {
    json_adapter_if_t::json_adapter_map_t res;
    res["type"] = boost::shared_ptr<json_adapter_if_t>(new json_temporary_adapter_t<std::string>("secondary_without_primary"));
    // TODO: git blame this and ask why it's commented out.
    //res["current_state"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<region_map_t<version_range_t> >(&target->current_state));
    //res["backfiller"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<backfiller_business_card_t>(&target->backfiller));
    return res;
}

cJSON *render_as_json(secondary_without_primary_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *, secondary_without_primary_t *) {
    throw permission_denied_exc_t("Can't write to primary_when_safe_t objects.\n");
}


// ctx-less json adapter for secondary_backfilling
json_adapter_if_t::json_adapter_map_t get_json_subfields(secondary_backfilling_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["type"] = boost::shared_ptr<json_adapter_if_t>(new json_temporary_adapter_t<std::string>("secondary_backfilling"));
    res["backfiller"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<backfill_location_t>(&target->backfill));
    return res;
}

cJSON *render_as_json(secondary_backfilling_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *, secondary_backfilling_t *) {
    throw permission_denied_exc_t("Can't write to secondary_backfilling objects.\n");
}


// ctx-less json adapter for nothing_when_safe_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(nothing_when_safe_t *) {
    json_adapter_if_t::json_adapter_map_t res;
    res["type"] = boost::shared_ptr<json_adapter_if_t>(new json_temporary_adapter_t<std::string>("nothing_when_safe"));
    // TODO: git blame this and ask why this is commented out.
    //res["current_state"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<region_map_t<version_range_t> >(&target->current_state));
    //res["backfiller"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<backfiller_business_card_t>(&target->backfiller));
    return res;
}


cJSON *render_as_json(nothing_when_safe_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *, nothing_when_safe_t *) {
    throw permission_denied_exc_t("Can't write to nothing_when_safe_t objects.\n");
}


// ctx-less json adapter for nothing_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(nothing_t *) {
    json_adapter_if_t::json_adapter_map_t res;
    res["type"] = boost::shared_ptr<json_adapter_if_t>(new json_temporary_adapter_t<std::string>("nothing"));
    return res;
}

cJSON *render_as_json(nothing_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *, nothing_t *) {
    throw permission_denied_exc_t("Can't write to nothing_t objects.\n");
}


// ctx-less json adapter for nothing_when_done_erasing_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(nothing_when_done_erasing_t *) {
    json_adapter_if_t::json_adapter_map_t res;
    res["type"] = boost::shared_ptr<json_adapter_if_t>(new json_temporary_adapter_t<std::string>("nothing_when_done_erasing"));
    return res;
}

cJSON *render_as_json(nothing_when_done_erasing_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *, nothing_when_done_erasing_t *) {
    throw permission_denied_exc_t("Can't write to nothing_when_done_erasing_t objects.\n");
}


} //namespace reactor_business_card_details


// json adapter for reactor_activity_entry_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(reactor_activity_entry_t *target) {
    // TODO: Rename these fields and rename this in the UI.
    json_adapter_if_t::json_adapter_map_t res;
    res["first"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<region_t>(&target->region));
    res["second"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<reactor_business_card_t::activity_t>(&target->activity));
    return res;
}

cJSON *render_as_json(reactor_activity_entry_t *target) {
    cJSON *res = cJSON_CreateArray();
    cJSON_AddItemToArray(res, render_as_json(&target->region));
    cJSON_AddItemToArray(res, render_as_json(&target->activity));
    return res;
}

void apply_json_to(cJSON *change, reactor_activity_entry_t *target) {
    json_array_iterator_t it = get_array_it(change);
    cJSON *first = it.next(), *second = it.next();
    if (!first || !second || it.next()) {
        throw schema_mismatch_exc_t("Expected an array with exactly 2 elements in it");
    }
    apply_json_to(first, &target->region);
    apply_json_to(second, &target->activity);
}


// ctx-less json adapter for reactor_business_card_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(reactor_business_card_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["activity_map"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<reactor_business_card_t::activity_map_t>(&target->activities));
    return res;
}

cJSON *render_as_json(reactor_business_card_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *change, reactor_business_card_t *target) {
    apply_as_directory(change, target);
}
