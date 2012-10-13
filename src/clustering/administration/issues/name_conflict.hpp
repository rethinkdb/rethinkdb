#ifndef CLUSTERING_ADMINISTRATION_ISSUES_NAME_CONFLICT_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_NAME_CONFLICT_HPP_

#include <list>
#include <set>
#include <string>

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/json.hpp"
#include "clustering/administration/metadata.hpp"
#include "http/json.hpp"

template <class> class semilattice_read_view_t;

class name_conflict_issue_t : public global_issue_t {
public:
    name_conflict_issue_t(
            const std::string &_type,
            const std::string &_contested_name,
            const std::set<uuid_t> &_contestants);

    std::string get_description() const;

    cJSON *get_json_description();

    name_conflict_issue_t *clone() const;

    // TODO: Why is type not an enumeration?
    // TODO: Why is every name conflict issue one of these things with "type"?
    std::string type;
    std::string contested_name;
    std::set<uuid_t> contestants;

private:
    DISABLE_COPYING(name_conflict_issue_t);
};

class name_conflict_issue_tracker_t : public global_issue_tracker_t {
public:
    explicit name_conflict_issue_tracker_t(boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _semilattice_view);
    ~name_conflict_issue_tracker_t();

    std::list<clone_ptr_t<global_issue_t> > get_issues();

private:
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_view;

    DISABLE_COPYING(name_conflict_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_NAME_CONFLICT_HPP_ */
