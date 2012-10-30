// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_JSON_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_JSON_HPP_

#include <string>

#include "containers/uuid.hpp"
#include "http/json/json_adapter.hpp"
#include "utils.hpp"

class issue_json_t {
public:
    bool critical;
    std::string description;
    std::string type;
    ticks_t time;

};

// ctx-less json adapter concept for issue_json_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(issue_json_t *);
cJSON *render_as_json(issue_json_t *);
void apply_json_to(cJSON *, issue_json_t *);
void on_subfield_change(issue_json_t *);

#endif  // CLUSTERING_ADMINISTRATION_ISSUES_JSON_HPP_
