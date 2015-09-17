// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <re2/re2.h>
#include <unicode/uchar.h>      // NOLINT(build/include_order)
#include <unicode/utypes.h>     // NOLINT(build/include_order)

#include <algorithm>

#include "parsing/utf8.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"

namespace ql {

// Combining characters in Unicode have a character class starting with M. There
// are three types, which affect layout; we don't distinguish between them here.
static bool is_combining_character(char32_t c) {
    return (U_GET_GC_MASK(c) & U_GC_M_MASK) > 0;
}

// ICU provides several different whitespace functions.  We use
// `u_isUWhiteSpace` which is the Unicode White_Space property.  It is
// highly unlikely that the details will matter, but for the record
// WSpace=Y includes characters that do not exist in the Z categories
// (Z designating separators) and some of the Z category marks do not
// have the WSpace=Y property; principally zero width spacers).
static bool is_whitespace_character(char32_t c) {
    return u_isUWhiteSpace(c);
}

class match_term_t : public op_term_t {
public:
    match_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        std::string str = args->arg(env, 0)->as_str().to_std();
        std::string re = args->arg(env, 1)->as_str().to_std();
        std::shared_ptr<re2::RE2> regexp;
        regex_cache_t &cache = env->env->regex_cache();
        auto search = cache.regexes.find(re);
        if (search == cache.regexes.end()) {
            regexp.reset(new re2::RE2(re, re2::RE2::Quiet));
            if (!regexp->ok()) {
                rfail(base_exc_t::LOGIC,
                      "Error in regexp `%s` (portion `%s`): %s",
                      regexp->pattern().c_str(),
                      regexp->error_arg().c_str(),
                      regexp->error().c_str());
            }
            cache.regexes[re] = regexp;
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

template <typename It>
It find_utf8_pred(It start, It end, std::function<bool(char32_t)> &&fn) {
    It pos(start);
    char32_t codepoint;
    while (pos != end) {
        It next = utf8::next_codepoint(pos, end, &codepoint);
        if (fn(codepoint)) {
            return pos;
        }
        pos = next;
    }
    return pos;
}
template <typename It> It find_space(It start, It end) {
    return find_utf8_pred(std::forward<It>(start), std::forward<It>(end), is_whitespace_character);
}
template <typename It>
It find_non_space(It start, It end) {
    return find_utf8_pred(start, end,
        [](char32_t ch) { return !is_whitespace_character(ch); });
}
template <typename It>
It find_non_combining(It start, It end) {
    return find_utf8_pred(start, end,
        [](char32_t ch) { return !is_combining_character(ch); });
}
template <typename It>
void push_datum(std::vector<datum_t> *res, It &&begin, It &&end) {
    res->push_back(datum_t(datum_string_t(
        std::string(std::forward<It>(begin), std::forward<It>(end)))));
}

class split_term_t : public op_term_t {
public:
    split_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1, 3)) { }
private:
    std::vector<datum_t> utf8_aware_split(const std::string &s,
                                          const boost::optional<std::string> &delim,
                                          size_t maxnum) const {
        const bool is_delim_empty = (delim && delim->size() == 0);
        std::vector<datum_t> res;
        std::string::const_iterator current = s.cbegin();
        std::string::const_iterator end = s.cend();
        bool done = false;
        while (!done) {
            if (res.size() == maxnum) {
                if (delim) {
                    if (delim->size() > 0 || current != end) {
                        push_datum(&res, current, end);
                    }
                } else {
                    auto start = find_non_space(current, end);
                    if (start != end) {
                        push_datum(&res, start, end);
                    }
                }
                current = end;
                done = true;
            } else if (is_delim_empty) {
                auto next = utf8::next_codepoint(current, end);
                auto with_combining = find_non_combining(next, end);

                if (current != with_combining) {
                    push_datum(&res, current, with_combining);
                }
                current = with_combining;
                done = (current == end);
            } else if (delim) {
                auto next = std::search(current, end, delim->begin(), delim->end());
                push_datum(&res, current, next);
                if (next == end) {
                    current = end;
                    done = true;
                } else {
                    current = next + delim->size();
                }
            } else {
                auto next = find_space(current, end);
                if (next == end) {
                    auto start = find_non_space(current, end);
                    if (start != end) {
                        push_datum(&res, start, end);
                    }
                    current = end;
                    done = true;
                } else {
                    if (current != next) {
                        push_datum(&res, current, next);
                    }
                    current = utf8::next_codepoint(next, end);
                }
            }
        }
        return res;
    }
    std::vector<datum_t> old_split(const std::string &s,
                                    const boost::optional<std::string> &delim,
                                    size_t maxnum) const {
        const char *const splitchars = " \t\n\r\x0B\x0C";
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
        return res;
    }
    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {
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
                   base_exc_t::LOGIC,
                   strprintf("Error: `split` size argument must be in range [-1, %zu].",
                             env->env->limits().array_size_limit() - 1));
        }
        size_t maxnum = (n < 0 ? std::numeric_limits<decltype(maxnum)>::max() : n);

        std::vector<datum_t> res;

        switch (env->env->reql_version()) {
        case reql_version_t::v1_14:
        case reql_version_t::v1_16: // v1_15 is the same as v1_14
        case reql_version_t::v2_0:
            res = old_split(s, delim, maxnum);
            break;
        case reql_version_t::v2_1:
        case reql_version_t::v2_2_is_latest:
            res = utf8_aware_split(s, delim, maxnum);
            break;
        default:
            unreachable();
        }

        return new_val(datum_t(std::move(res), env->env->limits()));
    }
    virtual const char *name() const { return "split"; }
};

counted_t<term_t> make_match_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<match_term_t>(env, term);
}
counted_t<term_t> make_split_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<split_term_t>(env, term);
}

}  // namespace ql
