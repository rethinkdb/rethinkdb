// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_

#include <list>
#include <set>
#include <string>

#include "clustering/administration/issues/json.hpp"
#include "concurrency/watchable.hpp"
#include "containers/clone_ptr.hpp"
#include "http/json.hpp"
#include "rpc/serialize_macros.hpp"

class local_issue_t {
public:
    local_issue_t() { }
    local_issue_t(const std::string& type, bool critical, const std::string& description);

    local_issue_t *clone() const;

    std::string type;
    bool critical;
    std::string description;
    int64_t timestamp;

    RDB_DECLARE_ME_SERIALIZABLE;
};

class local_issue_tracker_t {
public:
    local_issue_tracker_t() : issues_watchable(std::list<local_issue_t>()) { }

    class entry_t {
    public:
        entry_t(local_issue_tracker_t *p, const local_issue_t &d) :
            parent(p), issue(d)
        {
            parent->issues.insert(this);
            parent->recompute();
        }
        ~entry_t() {
            parent->issues.erase(this);
            parent->recompute();
        }
    private:
        friend class local_issue_tracker_t;
        local_issue_tracker_t *parent;
        local_issue_t issue;

        DISABLE_COPYING(entry_t);
    };

    clone_ptr_t<watchable_t<std::list<local_issue_t> > > get_issues_watchable() {
        return issues_watchable.get_watchable();
    }

private:
    void recompute() {
        std::list<local_issue_t> l;
        for (std::set<entry_t *>::iterator it = issues.begin(); it != issues.end(); it++) {
            l.push_back((*it)->issue);
        }
        issues_watchable.set_value(l);
    }

    std::set<entry_t *> issues;
    watchable_variable_t<std::list<local_issue_t> > issues_watchable;

    DISABLE_COPYING(local_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_ */
