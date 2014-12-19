// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <re2/re2.h>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"

namespace ql {

class match_term_t : public op_term_t {
public:
    match_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        std::string str = args->arg(env, 0)->as_str().to_std();
        std::string re = args->arg(env, 1)->as_str().to_std();
        std::shared_ptr<re2::RE2> regexp;
        query_cache_t & cache = env->env->query_cache();
        auto search = cache.regex_cache.find(re);
        if (search == cache.regex_cache.end()) {
            regexp.reset(new re2::RE2(re, re2::RE2::Quiet));
            if (!regexp->ok()) {
                rfail(base_exc_t::GENERIC,
                      "Error in regexp `%s` (portion `%s`): %s",
                      regexp->pattern().c_str(),
                      regexp->error_arg().c_str(),
                      regexp->error().c_str());
            }
            cache.regex_cache[re] = regexp;
        } else {
            regexp = search->second;
        }
        r_sanity_check(static_cast<bool>(regexp));
        // We add 1 to account for $0.
        int ngroups = regexp->NumberOfCapturingGroups() + 1;
        scoped_array_t<re2::StringPiece> groups(ngroups);
        if (regexp->Match(str, 0, str.size(), re2::RE2::UNANCHORED, groups.data(), ngroups)) {
            datum_object_builder_t match;
            // We use `b` to store whether or not we got a conflict when writing
            // to an object.  This should never happen here because we aren't
            // using user-generated keys, but the result of `add` is marked
            // MUST_USE.
            bool b = false;
            b |= match.add("str", datum_t(
                               datum_string_t(groups[0].as_string())));
            b |= match.add("start", datum_t(
                               static_cast<double>(groups[0].begin() - str.data())));
            b |= match.add("end", datum_t(
                               static_cast<double>(groups[0].end() - str.data())));
            datum_array_builder_t match_groups(env->env->limits());
            for (int i = 1; i < ngroups; ++i) {
                const re2::StringPiece &group = groups[i];
                if (group.data() == NULL) {
                    match_groups.add(datum_t::null());
                } else {
                    datum_object_builder_t match_group;
                    b |= match_group.add(
                        "str", datum_t(
                            datum_string_t(group.as_string())));
                    b |= match_group.add(
                        "start", datum_t(
                            static_cast<double>(group.begin() - str.data())));
                    b |= match_group.add(
                        "end", datum_t(
                            static_cast<double>(group.end() - str.data())));
                    match_groups.add(std::move(match_group).to_datum());
                }
            }
            b |= match.add("groups", std::move(match_groups).to_datum());
            r_sanity_check(!b);
            return new_val(std::move(match).to_datum());
        } else {
            return new_val(datum_t::null());
        }
    }
    virtual const char *name() const { return "match"; }
};

const char *const splitchars = " \t\n\r\x0B\x0C";

class split_term_t : public op_term_t {
public:
    split_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, 3)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        std::string s = args->arg(env, 0)->as_str().to_std();

        boost::optional<std::string> delim;
        if (args->num_args() > 1) {
            datum_t d = args->arg(env, 1)->as_datum();
            if (d.get_type() != datum_t::R_NULL) {
                delim = d.as_str().to_std();
            }
        }

        int64_t n = -1; // -1 means unlimited
        if (args->num_args() > 2) {
            n = args->arg(env, 2)->as_int();
            rcheck(n >= -1 && n <= int64_t(env->env->limits().array_size_limit()) - 1,
                   base_exc_t::GENERIC,
                   strprintf("Error: `split` size argument must be in range [-1, %zu].",
                             env->env->limits().array_size_limit() - 1));
        }
        size_t maxnum = (n < 0 ? std::numeric_limits<decltype(maxnum)>::max() : n);

        // This logic is extremely finicky so as to mimick the behavior of
        // Python's `split` in edge cases.
        std::vector<datum_t> res;
        size_t last = 0;
        while (last != std::string::npos) {
            size_t next = res.size() == maxnum
                ? std::string::npos
                : (delim
                   ? (delim->size() == 0 ? last + 1 : s.find(*delim, last))
                   : s.find_first_of(splitchars, last));
            std::string tmp;
            if (next == std::string::npos) {
                size_t start = delim ? last : s.find_first_not_of(splitchars, last);
                tmp = start == std::string::npos ? "" : s.substr(start);
            } else {
                tmp = s.substr(last, next - last);
            }
            if ((delim && delim->size() != 0) || tmp.size() != 0) {
                res.push_back(datum_t(datum_string_t(tmp)));
            }
            last = (next == std::string::npos || next >= s.size())
                ? std::string::npos
                : next + (delim ? delim->size() : 1);
        }

        return new_val(datum_t(std::move(res), env->env->limits()));
    }
    virtual const char *name() const { return "split"; }
};

counted_t<term_t> make_match_term(compile_env_t *env,
                                  const protob_t<const Term> &term) {
    return make_counted<match_term_t>(env, term);
}
counted_t<term_t> make_split_term(compile_env_t *env,
                                  const protob_t<const Term> &term) {
    return make_counted<split_term_t>(env, term);
}

}  // namespace ql
