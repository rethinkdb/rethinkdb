// Copyright 2010-2013 RethinkDB, all rights reserved.
#include <memory>
#include <string>

#include "clustering/administration/issues/json.hpp"

#include "http/json.hpp"
#include "http/json/json_adapter.hpp"

// json adapter concept for issue_json_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(issue_json_t *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["critical"] = std::shared_ptr<json_adapter_if_t>(new json_adapter_t<bool>(&target->critical));
    res["description"] = std::shared_ptr<json_adapter_if_t>(new json_adapter_t<std::string>(&target->description));
    res["type"] = std::shared_ptr<json_adapter_if_t>(new json_adapter_t<std::string>(&target->type));
    res["time"] = std::shared_ptr<json_adapter_if_t>(new json_adapter_t<ticks_t>(&target->time));

    return res;
}

cJSON *render_as_json(issue_json_t *target) {
    return render_as_directory(target);
}

void apply_json_to(cJSON *change, issue_json_t *target) {
    apply_as_directory(change, target);
}

void on_subfield_change(issue_json_t *) { }
