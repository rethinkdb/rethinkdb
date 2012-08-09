#ifndef CLUSTERING_ADMINISTRATION_ISSUES_GLOBAL_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_GLOBAL_HPP_

#include <list>
#include <set>
#include <string>

#include "containers/clone_ptr.hpp"
#include "http/json.hpp"
#include "http/json/json_adapter.hpp"

class global_issue_t {
public:
    virtual std::string get_description() const = 0;
    virtual cJSON *get_json_description() = 0;

    virtual ~global_issue_t() { }
    virtual global_issue_t *clone() const = 0;

    global_issue_t() { }

private:
    DISABLE_COPYING(global_issue_t);
};

class global_issue_tracker_t {
public:
    virtual std::list<clone_ptr_t<global_issue_t> > get_issues() = 0;

    global_issue_tracker_t() { }

protected:
    virtual ~global_issue_tracker_t() { }

private:
    DISABLE_COPYING(global_issue_tracker_t);
};

class global_issue_aggregator_t : public global_issue_tracker_t {
public:
    class source_t {
    public:
        source_t(global_issue_aggregator_t *_parent, global_issue_tracker_t *_source) :
            parent(_parent), source(_source) {
            parent->sources.insert(this);
        }
        ~source_t() {
            parent->sources.erase(this);
        }
    private:
        friend class global_issue_aggregator_t;
        global_issue_aggregator_t *parent;
        global_issue_tracker_t *source;
    };

    std::list<clone_ptr_t<global_issue_t> > get_issues() {
        std::list<clone_ptr_t<global_issue_t> > all;
        for (std::set<source_t *>::iterator it = sources.begin(); it != sources.end(); it++) {
            std::list<clone_ptr_t<global_issue_t> > from_source = (*it)->source->get_issues();
            all.splice(all.end(), from_source);
        }
        return all;
    }

    global_issue_aggregator_t() { }

private:
    std::set<source_t *> sources;

    DISABLE_COPYING(global_issue_aggregator_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_GLOBAL_HPP_ */
