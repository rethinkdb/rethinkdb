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

ql::datum_t perfmon_filter_t::filter(const ql::datum_t &stats) const {
    guarantee(f.has(), "perfmon_filter_t::filter was passed an uninitialized datum");
    return subfilter(stats, 0, std::vector<bool>(regexps.size(), true));
}

/* Filter a perfmon result.  [depth] is how deep we are in the paths that
   the [perfmon_filter_t] was constructed from, and [active] is the set of paths
   that are still active (i.e. that haven't failed a match yet).  This should
   only be called by [filter]. */
ql::datum_t perfmon_filter_t::subfilter(const ql::datum_t &stats,
                                        const size_t depth,
                                        const std::vector<bool> active) const {
    bool keep_this_perfmon = true;
    if (stats.get_type() == ql::datum_t::R_OBJECT) {
        ql::datum_object_builder_t builder;

        for (size_t i = 0; i < stats.obj_size(); ++i) {
            std::vector<bool> subactive = active;
            std::pair<datum_string_t, datum_t> pair = stats.get_pair(i);

            bool some_subpath = false;
            for (size_t j = 0; j < regexps.size(); ++j) {
                if (!active[j]) {
                    continue;
                }
                if (depth >= regexps[j].size()) {
                    continue;
                }
                subactive[j] = regexps[j][depth]->matches(pair.first.to_std());
                some_subpath |= subactive[j];
            }

            if (some_subpath) {
                builder.overwrite(pair.first,
                                  subfilter(pair.second, depth + 1, subactive));
            }
        }

        return std::move(builder).to_datum();
    }

    return stats;
}
