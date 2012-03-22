#ifndef CLUSTERING_ADMINISTRATION_ISSUES_VECTOR_CLOCK_CONFLICT_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_VECTOR_CLOCK_CONFLICT_HPP_

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"

class vector_clock_conflict_issue_t : public global_issue_t {
public:
    vector_clock_conflict_issue_t(
            const std::string &_object_type,
            const boost::uuids::uuid &_object_id,
            const std::string &_field) : 
        object_type(_object_type), object_id(_object_id), field(_field) { }

    std::string get_description() const {
        return "The " + object_type + " with UUID " + uuid_to_str(object_id) +
            " has a vector clock conflict in its field '" + field + "'.";
    }

    vector_clock_conflict_issue_t *clone() const {
        return new vector_clock_conflict_issue_t(object_type, object_id, field);
    }

    std::string object_type;
    boost::uuids::uuid object_id;
    std::string field;
};

class vector_clock_conflict_issue_tracker_t : public global_issue_tracker_t {
public:
    explicit vector_clock_conflict_issue_tracker_t(
            boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _semilattice_view) :
        semilattice_view(_semilattice_view) { }

    std::list<clone_ptr_t<global_issue_t> > get_issues();

private:
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_view;
};


#endif /* CLUSTERING_ADMINISTRATION_ISSUES_VECTOR_CLOCK_CONFLICT_HPP_ */
