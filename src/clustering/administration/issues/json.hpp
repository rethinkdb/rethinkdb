#ifndef __CLUSTERING_ADMINISTRATION_ISSUES_JSON_HPP__
#define __CLUSTERING_ADMINISTRATION_ISSUES_JSON_HPP__

#include <string>

#include "errors.hpp"
#include <boost/uuid/uuid.hpp>

#include "utils.hpp"
#include "http/json/json_adapter.hpp"

enum _issue_type_t {
    VCLOCK_CONFLICT,
    MACHINE_DOWN,
    NAME_CONFLICT_ISSUE,
    PERSISTENCE_ISSUE,
    PINNINGS_SHARDS_MISMATCH
};

class issue_type_t {
public:
    _issue_type_t issue_type;
};

//json adapter concept for issue_type_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(issue_type_t *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(issue_type_t *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, issue_type_t *, const ctx_t &);

template <class ctx_t>
void on_subfield_change(issue_type_t *, const ctx_t &);

class issue_json_t {
public:
    bool critical;
    std::string description;
    issue_type_t type;
    ticks_t time;
    
};

//json adapter concept for issue_json_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(issue_json_t *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(issue_json_t *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, issue_json_t *, const ctx_t &);

template <class ctx_t>
void on_subfield_change(issue_json_t *, const ctx_t &);

//A local issue occurs on a single machine
class local_issue_json_t : public issue_json_t {
public:
    boost::uuids::uuid machine;
};

//json adapter concept for local_issue_json_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(local_issue_json_t *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(local_issue_json_t *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, local_issue_json_t *, const ctx_t &);

template <class ctx_t>
void on_subfield_change(local_issue_json_t *, const ctx_t &);

#include "clustering/administration/issues/json.tcc"
#endif
