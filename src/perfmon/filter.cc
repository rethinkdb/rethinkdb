// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "perfmon/filter.hpp"

#include "errors.hpp"
#include <boost/tokenizer.hpp>

#include "containers/scoped_regex.hpp"
#include "logger.hpp"
#include "perfmon/core.hpp"
#include "utils.hpp"

/* Construct a filter from a set of paths.  Paths are of the form foo/bar/baz,
   where each of those can be a regular expression.  They act a lot like XPath
   expressions, but for perfmon_t objects.  */
perfmon_filter_t::perfmon_filter_t(const std::set<std::string> &paths) {
    typedef boost::escaped_list_separator<char> separator_t;
    typedef boost::tokenizer<separator_t> tokenizer_t;
    separator_t slashes("\\", "/", "");

    for (std::set<std::string>::const_iterator
             str = paths.begin(); str != paths.end(); ++str) {
        regexps.push_back(std::vector<scoped_regex_t *>());
        std::vector<scoped_regex_t *> *path = &regexps.back();
        try {
            tokenizer_t t(*str, slashes);
            for (tokenizer_t::const_iterator it = t.begin(); it != t.end(); ++it) {
                path->push_back(new scoped_regex_t());
                if (!path->back()->compile("^"+(*it)+"$")) {
                    logWRN("Error: regex %s failed to compile (%s), treating as empty.",
                           sanitize_for_logger(*it).c_str(),
                           sanitize_for_logger(path->back()->get_error()).c_str());
                    if (!path->back()->compile("^$")) {
                        crash("Regex '^$' failed to compile (%s).\n",
                              sanitize_for_logger(path->back()->get_error()).c_str());
                    }
                }
            }
        } catch (const boost::escaped_list_error &e) {
            logWRN("Error: Could not parse %s (%s), skipping.",
                   sanitize_for_logger(*str).c_str(), e.what());
            continue; //Skip this path
        }
    }
}

perfmon_filter_t::~perfmon_filter_t() {
    for (std::vector<std::vector<scoped_regex_t *> >::const_iterator
             it = regexps.begin(); it != regexps.end(); ++it) {
        for (std::vector<scoped_regex_t *>::const_iterator
                 regexp = it->begin(); regexp != it->end(); ++regexp) {
            delete *regexp;
        }
    }
}

void perfmon_filter_t::filter(const scoped_ptr_t<perfmon_result_t> *p) const {
    guarantee(p->has(), "perfmon_filter_t::filter needs a perfmon_result_t");
    subfilter(const_cast<scoped_ptr_t<perfmon_result_t> *>(p),
              0, std::vector<bool>(regexps.size(), true));
    guarantee(p->has(), "subfilter is not supposed to delete the top-most node.");
}

/* Filter a [perfmon_result_t].  [depth] is how deep we are in the paths that
   the [perfmon_filter_t] was constructed from, and [active] is the set of paths
   that are still active (i.e. that haven't failed a match yet).  This should
   only be called by [filter]. */
void perfmon_filter_t::subfilter(
    scoped_ptr_t<perfmon_result_t> *p_ptr, const size_t depth,
    const std::vector<bool> active) const {

    perfmon_result_t *const p = p_ptr->get();

    bool keep_this_perfmon = true;
    if (p->is_string()) {
        std::string *str = p->get_string();
        for (size_t i = 0; i < regexps.size(); ++i) {
            if (!active[i]) {
                continue;
            }
            if (depth >= regexps[i].size()) {
                return;
            }
            if (depth == regexps[i].size() && regexps[i][depth]->matches(*str)) {
                return;
            }
        }
        keep_this_perfmon = false;
    } else if (p->is_map()) {
        perfmon_result_t::iterator it = p->begin();
        while (it != p->end()) {
            std::vector<bool> subactive = active;
            bool some_subpath = false;
            for (size_t i = 0; i < regexps.size(); ++i) {
                if (!active[i]) {
                    continue;
                }
                if (depth >= regexps[i].size()) {
                    return;
                }
                subactive[i] = regexps[i][depth]->matches(it->first);
                some_subpath |= subactive[i];
            }
            if (some_subpath) {
                scoped_ptr_t<perfmon_result_t> tmp(it->second);
                subfilter(&tmp, depth + 1, subactive);
                it->second = tmp.release();
            }
            perfmon_result_t::iterator prev_it = it;
            ++it;
            if (!some_subpath || prev_it->second == NULL) {
                p->erase(prev_it);
            }
        }

        if (p->get_map_size() == 0) {
            keep_this_perfmon = false;
        }
    }

    if (!keep_this_perfmon && depth > 0) {  // Never delete the topmost node.
        p_ptr->reset();
    }
}
