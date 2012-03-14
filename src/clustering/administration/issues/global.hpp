#ifndef __CLUSTERING_ADMINISTRATION_ISSUES_GLOBAL_HPP__
#define __CLUSTERING_ADMINISTRATION_ISSUES_GLOBAL_HPP__

#include <list>
#include <set>
#include <string>

#include "containers/clone_ptr.hpp"

class global_issue_t {
public:
    virtual std::string get_description() const = 0;

    virtual ~global_issue_t() { }
    virtual global_issue_t *clone() const = 0;
};

class global_issue_tracker_t {
public:
    virtual std::list<clone_ptr_t<global_issue_t> > get_issues() = 0;

private:
    virtual ~global_issue_tracker_t() { }
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

private:
    std::set<source_t *> sources;
};

#endif /* __CLUSTERING_ADMINISTRATION_ISSUES_GLOBAL_HPP__ */
