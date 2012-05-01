#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_

#include <list>
#include <string>

#include "clustering/administration/issues/json.hpp"
#include "concurrency/watchable.hpp"
#include "containers/clone_ptr.hpp"
#include "http/json.hpp"
#include "rpc/serialize_macros.hpp"

class local_issue_t {
public:
    virtual std::string get_description() const = 0;
    virtual cJSON *get_json_description() const = 0;

    virtual ~local_issue_t() { }
    virtual local_issue_t *clone() const = 0;

    // Remember to include your subclass in the deserialize function below!
    enum serialization_code_t { PERSISTENCE_ISSUE_CODE };
    virtual void rdb_serialize(UNUSED write_message_t &msg) const = 0;

    local_issue_t() { }
private:
    DISABLE_COPYING(local_issue_t);
};

int deserialize(read_stream_t *s, local_issue_t **issue_ptr);

class local_issue_tracker_t {
public:
    local_issue_tracker_t() : issues_watchable(std::list<clone_ptr_t<local_issue_t> >()) { }

    class entry_t {
    public:
        entry_t(local_issue_tracker_t *p, const clone_ptr_t<local_issue_t> &d) :
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
        clone_ptr_t<local_issue_t> issue;
    };

    clone_ptr_t<watchable_t<std::list<clone_ptr_t<local_issue_t> > > > get_issues_watchable() {
        return issues_watchable.get_watchable();
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
    watchable_variable_t<std::list<clone_ptr_t<local_issue_t> > > issues_watchable;
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_ */
