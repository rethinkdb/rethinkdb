// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <string>

#include "clustering/administration/issues/json.hpp"

#include "http/json.hpp"
#include "http/json/json_adapter.hpp"

// json adapter concept for issue_json_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(issue_json_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["critical"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<bool>(&target->critical));
    res["description"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<std::string>(&target->description));
    res["type"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<std::string>(&target->type));
    res["time"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<ticks_t>(&target->time));

    return res;
}

cJSON *render_as_json(issue_json_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *change, issue_json_t *target) {
    apply_as_directory(change, target);
}

void on_subfield_change(issue_json_t *) { }
