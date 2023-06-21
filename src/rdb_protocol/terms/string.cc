// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <re2/re2.h>

#include <algorithm>

#include "parsing/utf8.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"

namespace ql {

// Closed interval ranges of combining chars.
struct closed_char32_range { char32_t lo, hi; };

// These huge-ass arrays were generated by running the old implementations of
// is_combining_character and is_whitespace_character on every single unicode character.
// (So we replicate exactly the same behavior as before.)

closed_char32_range combining_char_ranges[] = {
    {768, 879}, {1155, 1161}, {1425, 1469}, {1471, 1471}, {1473, 1474}, {1476, 1477}, {1479, 1479}, {1552, 1562}, {1611, 1631}, {1648, 1648}, {1750, 1756}, {1759, 1764}, {1767, 1768}, {1770, 1773}, {1809, 1809}, {1840, 1866}, {1958, 1968}, {2027, 2035}, {2070, 2073}, {2075, 2083}, {2085, 2087}, {2089, 2093}, {2137, 2139}, {2276, 2302}, {2304, 2307}, {2362, 2364}, {2366, 2383}, {2385, 2391}, {2402, 2403}, {2433, 2435}, {2492, 2492}, {2494, 2500}, {2503, 2504}, {2507, 2509}, {2519, 2519}, {2530, 2531}, {2561, 2563}, {2620, 2620}, {2622, 2626}, {2631, 2632}, {2635, 2637}, {2641, 2641}, {2672, 2673}, {2677, 2677}, {2689, 2691}, {2748, 2748}, {2750, 2757}, {2759, 2761}, {2763, 2765}, {2786, 2787}, {2817, 2819}, {2876, 2876}, {2878, 2884}, {2887, 2888}, {2891, 2893}, {2902, 2903}, {2914, 2915}, {2946, 2946}, {3006, 3010}, {3014, 3016}, {3018, 3021}, {3031, 3031}, {3073, 3075}, {3134, 3140}, {3142, 3144}, {3146, 3149}, {3157, 3158}, {3170, 3171}, {3202, 3203}, {3260, 3260}, {3262, 3268}, {3270, 3272}, {3274, 3277}, {3285, 3286}, {3298, 3299}, {3330, 3331}, {3390, 3396}, {3398, 3400}, {3402, 3405}, {3415, 3415}, {3426, 3427}, {3458, 3459}, {3530, 3530}, {3535, 3540}, {3542, 3542}, {3544, 3551}, {3570, 3571}, {3633, 3633}, {3636, 3642}, {3655, 3662}, {3761, 3761}, {3764, 3769}, {3771, 3772}, {3784, 3789}, {3864, 3865}, {3893, 3893}, {3895, 3895}, {3897, 3897}, {3902, 3903}, {3953, 3972}, {3974, 3975}, {3981, 3991}, {3993, 4028}, {4038, 4038}, {4139, 4158}, {4182, 4185}, {4190, 4192}, {4194, 4196}, {4199, 4205}, {4209, 4212}, {4226, 4237}, {4239, 4239}, {4250, 4253}, {4957, 4959}, {5906, 5908}, {5938, 5940}, {5970, 5971}, {6002, 6003}, {6068, 6099}, {6109, 6109}, {6155, 6157}, {6313, 6313}, {6432, 6443}, {6448, 6459}, {6576, 6592}, {6600, 6601}, {6679, 6683}, {6741, 6750}, {6752, 6780}, {6783, 6783}, {6912, 6916}, {6964, 6980}, {7019, 7027}, {7040, 7042}, {7073, 7085}, {7142, 7155}, {7204, 7223}, {7376, 7378}, {7380, 7400}, {7405, 7405}, {7410, 7412}, {7616, 7654}, {7676, 7679}, {8400, 8432}, {11503, 11505}, {11647, 11647}, {11744, 11775}, {12330, 12335}, {12441, 12442}, {42607, 42610}, {42612, 42621}, {42655, 42655}, {42736, 42737}, {43010, 43010}, {43014, 43014}, {43019, 43019}, {43043, 43047}, {43136, 43137}, {43188, 43204}, {43232, 43249}, {43302, 43309}, {43335, 43347}, {43392, 43395}, {43443, 43456}, {43561, 43574}, {43587, 43587}, {43596, 43597}, {43643, 43643}, {43696, 43696}, {43698, 43700}, {43703, 43704}, {43710, 43711}, {43713, 43713}, {43755, 43759}, {43765, 43766}, {44003, 44010}, {44012, 44013}, {64286, 64286}, {65024, 65039}, {65056, 65062}, {66045, 66045}, {68097, 68099}, {68101, 68102}, {68108, 68111}, {68152, 68154}, {68159, 68159}, {69632, 69634}, {69688, 69702}, {69760, 69762}, {69808, 69818}, {69888, 69890}, {69927, 69940}, {70016, 70018}, {70067, 70080}, {71339, 71351}, {94033, 94078}, {94095, 94098}, {119141, 119145}, {119149, 119154}, {119163, 119170}, {119173, 119179}, {119210, 119213}, {119362, 119364}, {917760, 917999}
};

closed_char32_range whitespace_ranges[] = {
    {9, 13}, {32, 32}, {133, 133}, {160, 160}, {5760, 5760}, {8192, 8202}, {8232, 8233},
    {8239, 8239}, {8287, 8287}, {12288, 12288}
};

bool is_in_a_range(closed_char32_range *begin, closed_char32_range *end, char32_t c) {
    // [begin, end)
    closed_char32_range *p = std::upper_bound(begin, end, closed_char32_range{c, 0},
                                              [](const closed_char32_range &x,
                                                 const closed_char32_range &y) {
                                                  return x.lo < y.lo;
                                              });

    if (p == begin) {
        return false;
    }
    --p;
    return p->lo <= c && c <= p->hi;
}

// Combining characters in Unicode have a character class starting with M. There
// are three types, which affect layout; we don't distinguish between them here.
static bool is_combining_character(char32_t c) {
    size_t n = sizeof(combining_char_ranges) / sizeof(combining_char_ranges[0]);
    return is_in_a_range(combining_char_ranges, combining_char_ranges + n, c);
}

// Equivalent to the ICU `u_isUWhiteSpace` which is the Unicode White_Space property.
static bool is_whitespace_character(char32_t c) {
    size_t n = sizeof(whitespace_ranges) / sizeof(whitespace_ranges[0]);
    return is_in_a_range(whitespace_ranges, whitespace_ranges + n, c);
}

class match_term_t : public op_term_t {
public:
    match_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        auto v0 = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }
        std::string str = v0->as_str().to_std();
        auto v1 = args->arg(err_out, env, 1);
        if (err_out->has()) { return noval(); }
        std::string re = v1->as_str().to_std();
        std::shared_ptr<re2::RE2> regexp;
        regex_cache_t &cache = env->env->regex_cache();
        std::shared_ptr<re2::RE2> *found;
        if (!cache.regexes.lookup(re, &found)) {
            regexp.reset(new re2::RE2(re, re2::RE2::Quiet));
            if (!regexp->ok()) {
                rfail(base_exc_t::LOGIC,
                      "Error in regexp `%s` (portion `%s`): %s",
                      regexp->pattern().c_str(),
                      regexp->error_arg().c_str(),
                      regexp->error().c_str());
            }
            cache.regexes.insert(re, regexp);
        } else {
            regexp = *found;
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
    return find_utf8_pred(std::forward<It>(start), std::forward<It>(end),
                          &is_whitespace_character);
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
                                          const optional<std::string> &delim,
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
                                    const optional<std::string> &delim,
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
        eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        auto v0 = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }
        std::string s = v0->as_str().to_std();

        optional<std::string> delim;
        if (args->num_args() > 1) {
            auto v1 = args->arg(err_out, env, 1);
            if (err_out->has()) { return noval(); }
            datum_t d = v1->as_datum();
            if (d.get_type() != datum_t::R_NULL) {
                delim.set(d.as_str().to_std());
            }
        }

        int64_t n = -1; // -1 means unlimited
        if (args->num_args() > 2) {
            auto v2 = args->arg(err_out, env, 2);
            if (err_out->has()) { return noval(); }
            n = v2->as_int();
            rcheck(n >= -1 && n <= int64_t(env->env->limits().array_size_limit()) - 1,
                   base_exc_t::LOGIC,
                   strprintf("Error: `split` size argument must be in range [-1, %zu].",
                             env->env->limits().array_size_limit() - 1));
        }
        size_t maxnum = (n < 0 ? std::numeric_limits<decltype(maxnum)>::max() : n);

        std::vector<datum_t> res;

        switch (env->env->reql_version()) {
        case reql_version_t::v1_16:
        case reql_version_t::v2_0:
            res = old_split(s, delim, maxnum);
            break;
        case reql_version_t::v2_1:
        case reql_version_t::v2_2:
        case reql_version_t::v2_3:
        case reql_version_t::v2_4_is_latest:
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
