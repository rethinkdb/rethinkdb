// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/date_time.hpp>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"
#include "rdb_protocol/pseudo_time.hpp"

namespace ql {

class iso8601_term_t : public op_term_t {
public:
    iso8601_term_t(visibility_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1), optargspec_t({"default_timezone"})) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<val_t> v = arg(env, 0);
        std::string tz = "";
        if (counted_t<val_t> vtz = optarg(env, "default_timezone")) {
            tz = vtz->as_str();
        }
        return new_val(pseudo::iso8601_to_time(v->as_str(), tz, v.get()));
    }
    virtual const char *name() const { return "iso8601"; }
};

class to_iso8601_term_t : public op_term_t {
public:
    to_iso8601_term_t(visibility_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return new_val(
            make_counted<const datum_t>(
                pseudo::time_to_iso8601(arg(env, 0)->as_ptype(pseudo::time_string))));
    }
    virtual const char *name() const { return "to_iso8601"; }
};

class epoch_time_term_t : public op_term_t {
public:
    epoch_time_term_t(visibility_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<val_t> v = arg(env, 0);
        return new_val(pseudo::make_time(v->as_num(), "+00:00"));
    }
    virtual const char *name() const { return "epoch_time"; }
};

class to_epoch_time_term_t : public op_term_t {
public:
    to_epoch_time_term_t(visibility_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return new_val(
            make_counted<const datum_t>(
                pseudo::time_to_epoch_time(arg(env, 0)->as_ptype(pseudo::time_string))));
    }
    virtual const char *name() const { return "to_epoch_time"; }
};

class now_term_t : public op_term_t {
public:
    now_term_t(visibility_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(0)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *, UNUSED eval_flags_t flags) {
        // This should never get called because we rewrite `now` calls to a
        // constant so that they're deterministic.
        // RSI: this is a code smell, instead the deterministic value should be stored in env_t.
        r_sanity_check(false);
        unreachable();
    }
    bool is_deterministic() const { return false; }
    virtual const char *name() const { return "now"; }
};

class in_timezone_term_t : public op_term_t {
public:
    in_timezone_term_t(visibility_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return new_val(pseudo::time_in_tz(arg(env, 0)->as_ptype(pseudo::time_string),
                                          arg(env, 1)->as_datum()));
    }
    virtual const char *name() const { return "in_timezone"; }
};

class during_term_t : public bounded_op_term_t {
public:
    during_term_t(visibility_env_t *env, const protob_t<const Term> &term)
        : bounded_op_term_t(env, term, argspec_t(3)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<const datum_t> t = arg(env, 0)->as_ptype(pseudo::time_string);
        counted_t<const datum_t> lb = arg(env, 1)->as_ptype(pseudo::time_string);
        counted_t<const datum_t> rb = arg(env, 2)->as_ptype(pseudo::time_string);
        int lcmp = pseudo::time_cmp(*lb, *t);
        int rcmp = pseudo::time_cmp(*t, *rb);
        return new_val_bool(!(lcmp == 1 || (lcmp == 0 && left_open(env))
                              || rcmp == 1 || (rcmp == 0 && right_open(env))));
    }
    virtual const char *name() const { return "during"; }
};

class date_term_t : public op_term_t {
public:
    date_term_t(visibility_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return new_val(pseudo::time_date(arg(env, 0)->as_ptype(pseudo::time_string), this));
    }
    virtual const char *name() const { return "date"; }
};

class time_of_day_term_t : public op_term_t {
public:
    time_of_day_term_t(visibility_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return new_val(pseudo::time_of_day(arg(env, 0)->as_ptype(pseudo::time_string)));
    }
    virtual const char *name() const { return "time_of_day"; }
};

class timezone_term_t : public op_term_t {
public:
    timezone_term_t(visibility_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return new_val(pseudo::time_tz(arg(env, 0)->as_ptype(pseudo::time_string)));
    }
    virtual const char *name() const { return "timezone"; }
};

class portion_term_t : public op_term_t {
public:
    portion_term_t(visibility_env_t *env, protob_t<const Term> term,
                   pseudo::time_component_t _component)
        : op_term_t(env, term, argspec_t(1)), component(_component) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        double d = pseudo::time_portion(arg(env, 0)->as_ptype(pseudo::time_string), component);
        return new_val(make_counted<const datum_t>(d));
    }
    virtual const char *name() const {
        switch (component) {
        case pseudo::YEAR: return "year";
        case pseudo::MONTH: return "month";
        case pseudo::DAY: return "day";
        case pseudo::DAY_OF_WEEK: return "day_of_week";
        case pseudo::DAY_OF_YEAR: return "day_of_year";
        case pseudo::HOURS: return "hours";
        case pseudo::MINUTES: return "minutes";
        case pseudo::SECONDS: return "seconds";
        default: unreachable();
        }
    }
    pseudo::time_component_t component;
};

class time_term_t : public op_term_t {
public:
    time_term_t(visibility_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(4, 7)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        rcheck(num_args() == 4 || num_args() == 7, base_exc_t::GENERIC,
               strprintf("Got %zu arguments to TIME (expected 4 or 7).", num_args()));
        int year = arg(env, 0)->as_int<int>();
        int month = arg(env, 1)->as_int<int>();
        int day = arg(env, 2)->as_int<int>();
        int hours = 0;
        int minutes = 0;
        double seconds = 0;
        std::string tz = "";
        if (num_args() == 4) {
            tz = parse_tz(arg(env, 3));
        } else if (num_args() == 7) {
            hours = arg(env, 3)->as_int<int>();
            minutes = arg(env, 4)->as_int<int>();
            seconds = arg(env, 5)->as_num();
            tz = parse_tz(arg(env, 6));
        } else {
            r_sanity_check(false);
        }
        return new_val(
            pseudo::make_time(year, month, day, hours, minutes, seconds, tz, this));
    }
    std::string parse_tz(counted_t<val_t> v) {
        counted_t<const datum_t> d = v->as_datum();
        return d->as_str();
    }
    virtual const char *name() const { return "time"; }
};

counted_t<term_t> make_iso8601_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<iso8601_term_t>(env, term);
}
counted_t<term_t> make_to_iso8601_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<to_iso8601_term_t>(env, term);
}
counted_t<term_t> make_epoch_time_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<epoch_time_term_t>(env, term);
}
counted_t<term_t> make_to_epoch_time_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<to_epoch_time_term_t>(env, term);
}
counted_t<term_t> make_now_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<now_term_t>(env, term);
}
counted_t<term_t> make_in_timezone_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<in_timezone_term_t>(env, term);
}
counted_t<term_t> make_during_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<during_term_t>(env, term);
}

counted_t<term_t> make_date_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<date_term_t>(env, term);
}
counted_t<term_t> make_time_of_day_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<time_of_day_term_t>(env, term);
}
counted_t<term_t> make_timezone_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<timezone_term_t>(env, term);
}
counted_t<term_t> make_time_term(visibility_env_t *env, const protob_t<const Term> &term) {
    return make_counted<time_term_t>(env, term);
}
counted_t<term_t> make_portion_term(visibility_env_t *env, const protob_t<const Term> &term,
                                    pseudo::time_component_t component) {
    return make_counted<portion_term_t>(env, term, component);
}

} // namespace ql

