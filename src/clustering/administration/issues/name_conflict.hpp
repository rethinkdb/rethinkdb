#ifndef CLUSTERING_ADMINISTRATION_ISSUES_NAME_CONFLICT_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_NAME_CONFLICT_HPP_

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/metadata.hpp"
#include "rpc/semilattice/view.hpp"
#include "clustering/administration/issues/json.hpp"

class name_conflict_issue_t : public global_issue_t {
public:
    name_conflict_issue_t(
            const std::string &_type,
            const std::string &_contested_name,
            const std::set<boost::uuids::uuid> &_contestants) : 
        type(_type), contested_name(_contested_name), contestants(_contestants) { }

    std::string get_description() const {
        std::string message = "The following " + type + "s are all named '" + contested_name + "': ";
        for (std::set<boost::uuids::uuid>::iterator it = contestants.begin(); it != contestants.end(); it++) {
            message += uuid_to_str(*it) + "; ";
        }
        return message;
    }

    cJSON *get_json_description() const {
        issues::issue_json_t json;
        json.critical = false;
        json.description = "The following " + type + "s are all named '" + contested_name + "': ";
        for (std::set<boost::uuids::uuid>::iterator it = contestants.begin(); it != contestants.end(); it++) {
            json.description += uuid_to_str(*it) + "; ";
        }
        json.type.issue_type = issues::NAME_CONFLICT_ISSUE;
        json.time = get_ticks();

        return issues::render_as_json<int>(&json, 0);
    }

    name_conflict_issue_t *clone() const {
        return new name_conflict_issue_t(type, contested_name, contestants);
    }

    std::string type;
    std::string contested_name;
    std::set<boost::uuids::uuid> contestants;
};

class name_conflict_issue_tracker_t : public global_issue_tracker_t {
public:
    explicit name_conflict_issue_tracker_t(
            boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _semilattice_view) :
        semilattice_view(_semilattice_view) { }

    std::list<clone_ptr_t<global_issue_t> > get_issues();

private:
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_view;
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_NAME_CONFLICT_HPP_ */
