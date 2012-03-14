#ifndef __CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP__
#define __CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP__

#include <list>
#include <string>

#include "concurrency/watchable.hpp"
#include "containers/clone_ptr.hpp"

class local_issue_t {
public:
    virtual std::string get_description() const = 0;

    virtual ~local_issue_t() { }
    virtual local_issue_t *clone() const = 0;
};

class local_issue_tracker_t {
public:
    class entry_t {
    public:
        entry_t(local_issue_tracker_t *p, const clone_ptr_t<local_issue_t> &d) :
            parent(p) {
            parent->issues.insert(this);
            parent->recompute();
        }
        ~entry_t() {
            parent->issues.remove(this);
            parent->recompute();
        }
    private:
        local_issue_tracker_t *parent;
        clone_ptr_t<local_issue_t> issue;
    };

    watchable_t<std::list<clone_ptr_t<local_issue_t> > > *get_issues_watchable() {
        return &issues_watchable;
    }

private:
    void recompute() {
        std::list<clone_ptr_t<local_issue_t> > l;
        for (std::set<entry_t *>::iterator it = issues.begin(); it != issues.end(); it++) {
            l.push_back((*it)->issue);
        }
        issues_watchable.set_value(l);
    }

    std::set<entry_t *> issues;
    watchable_impl_t<std::list<clone_ptr_t<local_issue_t> > > issues_watchable;
};

#endif /* __CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP__ */
