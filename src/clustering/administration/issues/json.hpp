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

//json adapter concept for issue_json_t
template <class ctx_t>
typename json_adapter_if_t<ctx_t>::json_adapter_map_t get_json_subfields(issue_json_t *, const ctx_t &);

template <class ctx_t>
cJSON *render_as_json(issue_json_t *, const ctx_t &);

template <class ctx_t>
void apply_json_to(cJSON *, issue_json_t *, const ctx_t &);

template <class ctx_t>
void on_subfield_change(issue_json_t *, const ctx_t &);

// TODO: Why the fuck is this inheriting instead of using composition?
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



#endif  // CLUSTERING_ADMINISTRATION_ISSUES_JSON_HPP_
