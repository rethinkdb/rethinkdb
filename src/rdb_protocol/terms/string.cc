// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <re2/re2.h>
#include <unicode/uchar.h>
#include <unicode/utypes.h>

#include <algorithm>

#include "parsing/utf8.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"

namespace ql {

// Combining characters in Unicode have a character class starting with M. There
// are three types, which affect layout; we don't distinguish between them here.
static bool is_combining_character(char32_t c) {
    return (U_GET_GC_MASK(c) & U_GC_M_MASK);
}

// ICU provides several different whitespace functions.  We use
// `u_isUWhiteSpace` which is the Unicode White_Space property.  It is highly
// unlikely that the details will matter.
static bool is_whitespace_character(char32_t c) {
    return (U_GET_GC_MASK(c) & U_GC_M_MASK);
}

class match_term_t : public op_term_t {
public:
    match_term_t(compile_env_t *env, const protob_t<const Term> &term)
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
                rfail(base_exc_t::GENERIC,
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
It find_utf8_pred(It &&start, It &&end, std::function<bool(char32_t)> &&fn) {
    It pos(start);
    char32_t codepoint;
    utf8::reason_t reason;
    while (pos != end) {
        It next = utf8::next_codepoint(std::forward<It>(pos),
                                       std::forward<It>(end), &codepoint,
                                       &reason);
        if (fn(codepoint)) {
            break;
        }
        pos = next;
    }
    return pos;
}
template <typename It> It find_space(It &&start, It &&end) {
    return find_utf8_pred(std::forward<It>(start), std::forward<It>(end), is_whitespace_character);
}
template <typename It>
It find_non_space(It &&start, It &&end) {
    return find_utf8_pred(
        std::forward<It>(start), std::forward<It>(end),
        [](char32_t ch) { return !is_whitespace_character(ch); });
}
template <typename It>
It find_non_combining(It &&start, It &&end) {
    return find_utf8_pred(
        std::forward<It>(start), std::forward<It>(end),
        [](char32_t ch) { return !is_combining_character(ch); });
}
template <typename It>
void push_datum(std::vector<datum_t> *res, It &&begin, It &&end) {
    res->push_back(datum_t(datum_string_t(
        std::string(std::forward<It>(begin), std::forward<It>(end)))));
}

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
        const bool is_delim_empty = (delim && delim->size() == 0);

        int64_t n = -1; // -1 means unlimited
        if (args->num_args() > 2) {
            n = args->arg(env, 2)->as_int();
            rcheck(n >= -1 && n <= int64_t(env->env->limits().array_size_limit()) - 1,
                   base_exc_t::GENERIC,
                   strprintf("Error: `split` size argument must be in range [-1, %zu].",
                             env->env->limits().array_size_limit() - 1));
        }
        size_t maxnum = (n < 0 ? std::numeric_limits<decltype(maxnum)>::max() : n);

        std::vector<datum_t> res;
        std::string::const_iterator current = s.cbegin();
        std::string::const_iterator end = s.cend();
        while (current != end) {
            if (res.size() == maxnum) {
                auto it = delim ? current : find_non_space(current, end);
                if (it != s.end()) {
                    push_datum(&res, it, end);
                }
                current = s.end();
            } else if (is_delim_empty) {
                auto ch = utf8::next_codepoint(current, end);
                auto with_combining = find_non_combining(ch, end);
                // empty delimiters are special; need to push even empty
                // strings.
                push_datum(&res, current, with_combining);
                current = with_combining;
            } else if (delim) {
                auto next_delim = std::search(current, end,
                                              delim->begin(), delim->end());
                if (current != next_delim) {
                    push_datum(&res, current, next_delim);
                }
                current = next_delim == end ? end : next_delim + delim->size();
            } else {
                auto it = find_space(current, end);
                if (current != it) {
                    push_datum(&res, current, it);
                }
                current = find_non_space(it, end);
            }
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
