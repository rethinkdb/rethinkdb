#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"
#include <re2/re2.h>

namespace ql {

class match_term_t : public op_term_t {
public:
    match_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        std::string str = arg(0)->as_str();
        RE2 regexp(arg(1)->as_str());
        if (!regexp.ok()) {
            rfail(base_exc_t::GENERIC,
                  "Error in regexp `%s` (portion `%s`): %s",
                  regexp.pattern().c_str(),
                  regexp.error_arg().c_str(),
                  regexp.error().c_str());
        }
        // We add 1 to account for $0.
        int ngroups = regexp.NumberOfCapturingGroups() + 1;
        scoped_array_t<re2::StringPiece> groups(ngroups);
        if (regexp.Match(str, 0, str.size(), RE2::UNANCHORED, groups.data(), ngroups)) {
            scoped_ptr_t<datum_t> match(new datum_t(datum_t::R_OBJECT));
            // We use `b` to store whether or not we got a conflict when writing
            // to an object.  This should never happen here because we aren't
            // using user-generated keys, but the result of `add` is marked
            // MUST_USE.
            bool b = false;
            b |= match->add("str", make_counted<const datum_t>(groups[0].as_string()));
            b |= match->add("start", make_counted<const datum_t>(
                                static_cast<double>(groups[0].begin() - str.data())));
            b |= match->add("end", make_counted<const datum_t>(
                                static_cast<double>(groups[0].end() - str.data())));
            scoped_ptr_t<datum_t> match_groups(new datum_t(datum_t::R_ARRAY));
            for (int i = 1; i < ngroups; ++i) {
                const re2::StringPiece &group = groups[i];
                if (group.data() == NULL) {
                    match_groups->add(make_counted<datum_t>(datum_t::R_NULL));
                } else {
                    scoped_ptr_t<datum_t> match_group(new datum_t(datum_t::R_OBJECT));
                    b |= match_group->add(
                        "str", make_counted<const datum_t>(group.as_string()));
                    b |= match_group->add(
                        "start", make_counted<const datum_t>(
                            static_cast<double>(group.begin() - str.data())));
                    b |= match_group->add(
                        "end", make_counted<const datum_t>(
                            static_cast<double>(group.end() - str.data())));
                    match_groups->add(counted_t<const datum_t>(match_group.release()));
                }
            }
            b |= match->add("groups", counted_t<const datum_t>(match_groups.release()));
            r_sanity_check(!b);
            return new_val(counted_t<const datum_t>(match.release()));
        } else {
            return new_val(make_counted<const datum_t>(datum_t::R_NULL));
        }
    }
    virtual const char *name() const { return "match"; }
};

counted_t<term_t> make_match_term(env_t *env, protob_t<const Term> term) {
    return make_counted<match_term_t>(env, term);
}

}
