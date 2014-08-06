// Copyright 2014-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_

#include <list>
#include <set>
#include <string>

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/json.hpp"
#include "clustering/administration/metadata.hpp"
#include "http/json.hpp"

template <class> class semilattice_read_view_t;

class outdated_index_issue_t : public global_issue_t {
public:
    outdated_index_issue_t(std::map<namespace_id_t, std::set<std::string> > &&indexes);
    std::string get_description() const;
    cJSON *get_json_description();
    outdated_index_issue_t *clone() const;

    std::map<namespace_id_t, std::set<std::string> > outdated_indexes;

private:
    DISABLE_COPYING(outdated_index_issue_t);
};

class outdated_index_issue_tracker_t : public global_issue_tracker_t {
public:
    outdated_index_issue_tracker_t();
    ~outdated_index_issue_tracker_t();

    std::set<std::string> *get_index_set(const namespace_id_t &ns_id);
    std::list<clone_ptr_t<global_issue_t> > get_issues();

private:
    std::map<namespace_id_t, std::set<std::string> > collect_and_merge_maps();
    one_per_thread_t<std::map<namespace_id_t, std::set<std::string> > > outdated_indexes;

    DISABLE_COPYING(outdated_index_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_ */
