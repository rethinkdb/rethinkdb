#ifndef CLUSTERING_ADMINISTRATION_ISSUES_VECTOR_CLOCK_CONFLICT_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_VECTOR_CLOCK_CONFLICT_HPP_

#include <list>
#include <string>

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/json.hpp"
#include "clustering/administration/metadata.hpp"

template <class metadata_t> class semilattice_read_view_t;

class vector_clock_conflict_issue_t : public global_issue_t {
public:
    vector_clock_conflict_issue_t(
            const std::string &_object_type,
            const uuid_t &_object_id,
            const std::string &_field) :
        object_type(_object_type), object_id(_object_id), field(_field) { }

    std::string get_description() const {
        return "The " + object_type + " with UUID " + uuid_to_str(object_id) +
            " has a vector clock conflict in its field '" + field + "'.";
    }

    cJSON *get_json_description() {
        issue_json_t json;
        json.critical = false;
        json.time = get_secs();
        json.description = "The " + object_type + " with UUID " + uuid_to_str(object_id) +
            " has a vector clock conflict in its field '" + field + "'.";
        json.type = "VCLOCK_CONFLICT";

        cJSON *res = render_as_json(&json);

        cJSON_AddItemToObject(res, "object_type", render_as_json(&object_type));
        cJSON_AddItemToObject(res, "object_id", render_as_json(&object_id));
        cJSON_AddItemToObject(res, "field", render_as_json(&field));

        return res;
    }

    vector_clock_conflict_issue_t *clone() const {
        return new vector_clock_conflict_issue_t(object_type, object_id, field);
    }

    std::string object_type;
    uuid_t object_id;
    std::string field;

private:
    DISABLE_COPYING(vector_clock_conflict_issue_t);
};

class vector_clock_conflict_issue_tracker_t : public global_issue_tracker_t {
public:
    explicit vector_clock_conflict_issue_tracker_t(boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _semilattice_view);
    ~vector_clock_conflict_issue_tracker_t();

    std::list<clone_ptr_t<global_issue_t> > get_issues();
    std::list<clone_ptr_t<vector_clock_conflict_issue_t> > get_vector_clock_issues();

private:
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_view;

    DISABLE_COPYING(vector_clock_conflict_issue_tracker_t);
};


#endif /* CLUSTERING_ADMINISTRATION_ISSUES_VECTOR_CLOCK_CONFLICT_HPP_ */
