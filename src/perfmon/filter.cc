// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "perfmon/filter.hpp"

#include "errors.hpp"
#include <boost/tokenizer.hpp>

#include "logger.hpp"
#include "perfmon/core.hpp"
#include "utils.hpp"

/* Construct a filter from a set of paths.  Paths are of the form foo/bar/baz,
   where each of those can be a regular expression.  They act a lot like XPath
   expressions, but for perfmon_t objects.  */
perfmon_filter_t::perfmon_filter_t(const std::set<std::vector<std::string> > &paths) {
    for (auto const &path : paths) {
        std::vector<scoped_ptr_t<RE2> > compiled_path;
        for (auto const &str : path) {
            scoped_ptr_t<RE2> re = make_scoped<RE2>("^" + str + "$");
            if (!re->ok()) {
                logWRN("Error: regex %s failed to compile, treating as empty.",
                       sanitize_for_logger(str).c_str());
				re = scoped_ptr_t<RE2>(new RE2("^$"));
                if (!re->ok()) {
                    crash("Regex '^$' failed to compile.\n");
                }
            }

            compiled_path.emplace_back(std::move(re));
        }
        regexps.emplace_back(std::move(compiled_path));
    }
}

ql::datum_t perfmon_filter_t::filter(const ql::datum_t &stats) const {
    guarantee(stats.has(), "perfmon_filter_t::filter was passed an uninitialized datum");
    return subfilter(stats, 0, std::vector<bool>(regexps.size(), true));
}

/* Filter a perfmon result.  [depth] is how deep we are in the paths that
   the [perfmon_filter_t] was constructed from, and [active] is the set of paths
   that are still active (i.e. that haven't failed a match yet).  This should
   only be called by [filter]. */
ql::datum_t perfmon_filter_t::subfilter(const ql::datum_t &stats,
                                        const size_t depth,
                                        const std::vector<bool> active) const {
    if (stats.get_type() == ql::datum_t::R_OBJECT) {
        ql::datum_object_builder_t builder;

        for (size_t i = 0; i < stats.obj_size(); ++i) {
            std::vector<bool> subactive = active;
            std::pair<datum_string_t, ql::datum_t> pair = stats.get_pair(i);

            bool some_subpath = false;
            for (size_t j = 0; j < regexps.size(); ++j) {
                if (!active[j]) {
                    continue;
                }
                if (depth >= regexps[j].size()) {
                    some_subpath = true;
                    continue;
                }
                subactive[j] = RE2::FullMatch(pair.first.to_std(), *regexps[j][depth]);
                some_subpath |= subactive[j];
            }

            // Only write the stats if there was a match somewhere down the tree
            if (some_subpath) {
                ql::datum_t sub_stats = subfilter(pair.second, depth + 1, subactive);
                if (sub_stats.get_type() != ql::datum_t::R_OBJECT ||
                    sub_stats.obj_size() > 0) {
                    builder.overwrite(pair.first, sub_stats);
                }
            }
        }

        return std::move(builder).to_datum();
    }

    return stats;
}
