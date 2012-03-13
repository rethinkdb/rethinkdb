#ifndef __CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_TRACKER_HPP__
#define __CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_TRACKER_HPP__

#include "clustering/administration/issues/issue.hpp"

class local_issue_t {
public:
    virtual std::string get_description() const = 0;

    virtual local_issue_t *clone() const = 0;
};

class local_issue_tracker_t {
public:
    class entry_t {
    public:
        entry_t(local_issue_tracker_t *p, const clone_ptr_t<local_issue_t> &d);
        ~entry_t();
    private:
        issue_tracker_t *parent;
        std::list<clone_ptr_t<issue_description_t> >::iterator our_list_entry;
    };

    watchable_t<std::list<clone_ptr_t<issue_description_t> > > *get_issues_watchable();

private:
    std::list<clone_ptr_t<issue_description_t> >::iterator list;
    watchable_impl_t<std::list<clone_ptr_t<issue_description_t> > > issues_watchable;
};

#endif /* __CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_TRACKER_HPP__ */
