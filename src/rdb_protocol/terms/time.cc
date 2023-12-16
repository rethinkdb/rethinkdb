// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/date_time.hpp>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"
#include "rdb_protocol/pseudo_time.hpp"

namespace ql {

class iso8601_term_t : public op_term_t {
public:
    iso8601_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1), optargspec_t({"default_timezone"})) { }
private:
    scoped_ptr_t<val_t> eval_impl(eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }
        std::string tz = "";
        {
            scoped_ptr_t<val_t> vtz = args->optarg(err_out, env, "default_timezone");
            if (err_out->has()) { return noval(); }
            if (vtz) {
                tz = vtz->as_str().to_std();
            }
        }
        return new_val(pseudo::iso8601_to_time(
            env->env->reql_version(), v->as_str().to_std(), tz, v.get()));
    }
    virtual const char *name() const { return "iso8601"; }
};

class to_iso8601_term_t : public op_term_t {
public:
    to_iso8601_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    scoped_ptr_t<val_t> eval_impl(eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        auto v0 = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }
        return new_val(
            datum_t(datum_string_t(
                pseudo::time_to_iso8601(
                    env->env->reql_version(),
                    v0->as_ptype(pseudo::time_string)))));
    }
    virtual const char *name() const { return "to_iso8601"; }
};

class epoch_time_term_t : public op_term_t {
public:
    epoch_time_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    scoped_ptr_t<val_t> eval_impl(eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }
        return new_val(pseudo::make_time(v->as_num(), "+00:00"));
    }
    virtual const char *name() const { return "epoch_time"; }
};

class to_epoch_time_term_t : public op_term_t {
public:
    to_epoch_time_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    scoped_ptr_t<val_t> eval_impl(eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        auto v0 = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }
        return new_val(
            datum_t(
                pseudo::time_to_epoch_time(v0->as_ptype(pseudo::time_string))));
    }
    virtual const char *name() const { return "to_epoch_time"; }
};

class now_term_t : public op_term_t {
public:
    now_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(0)) { }
private:
    scoped_ptr_t<val_t> eval_impl(eval_error *, scope_env_t *env, args_t *, eval_flags_t) const {
        // Return the deterministic time from the env
        r_sanity_check(env->env->get_deterministic_time().has());
        return new_val(env->env->get_deterministic_time());
    }
    virtual deterministic_t is_deterministic() const { return deterministic_t::constant_now(); }
    virtual const char *name() const { return "now"; }
};

class in_timezone_term_t : public op_term_t {
public:
    in_timezone_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    scoped_ptr_t<val_t> eval_impl(eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        auto v0 = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }

        auto v1 = args->arg(err_out, env, 1);
        if (err_out->has()) { return noval(); }

        return new_val(pseudo::time_in_tz(v0->as_ptype(pseudo::time_string),
                                          v1->as_datum()));
    }
    virtual const char *name() const { return "in_timezone"; }
};

class during_term_t : public bounded_op_term_t {
public:
    during_term_t(compile_env_t *env, const raw_term_t &term)
        : bounded_op_term_t(env, term, argspec_t(3)) { }
private:
    scoped_ptr_t<val_t> eval_impl(eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        auto v0 = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }
        datum_t t = v0->as_ptype(pseudo::time_string);

        auto v1 = args->arg(err_out, env, 1);
        if (err_out->has()) { return noval(); }
        datum_t lb = v1->as_ptype(pseudo::time_string);

        auto v2 = args->arg(err_out, env, 2);
        if (err_out->has()) { return noval(); }
        datum_t rb = v2->as_ptype(pseudo::time_string);
        int lcmp = pseudo::time_cmp(lb, t);
        int rcmp = pseudo::time_cmp(t, rb);
        bool negret = false;
        if (lcmp > 0) {
            negret = true;
        } else {
            if (lcmp == 0) {
                bool left_open = is_left_open(err_out, env, args);
                if (err_out->has()) { return noval(); }
                if (left_open) {
                    negret = true;
                    goto done;
                }
            }

            if (rcmp > 0) {
                negret = true;
            } else {
                if (rcmp == 0) {
                    bool right_open = is_right_open(err_out, env, args);
                    if (err_out->has()) { return noval(); }
                    negret = right_open;
                } else {
                    negret = false;
                }
            }
        }
    done:
        return new_val_bool(!negret);
    }
    virtual const char *name() const { return "during"; }
};

class date_term_t : public op_term_t {
public:
    date_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    scoped_ptr_t<val_t> eval_impl(eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        auto v0 = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }

        return new_val(pseudo::time_date(
            env->env->reql_version(),
            v0->as_ptype(pseudo::time_string), this));
    }
    virtual const char *name() const { return "date"; }
};

class time_of_day_term_t : public op_term_t {
public:
    time_of_day_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    scoped_ptr_t<val_t> eval_impl(eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        auto v0 = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }

        return new_val(pseudo::time_of_day(
            env->env->reql_version(),
            v0->as_ptype(pseudo::time_string)));
    }
    virtual const char *name() const { return "time_of_day"; }
};

class timezone_term_t : public op_term_t {
public:
    timezone_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    scoped_ptr_t<val_t> eval_impl(eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        auto v0 = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }

        return new_val(pseudo::time_tz(v0->as_ptype(pseudo::time_string)));
    }
    virtual const char *name() const { return "timezone"; }
};

class portion_term_t : public op_term_t {
public:
    portion_term_t(compile_env_t *env, const raw_term_t &term,
                   pseudo::time_component_t _component)
        : op_term_t(env, term, argspec_t(1)), component(_component) { }
private:
    scoped_ptr_t<val_t> eval_impl(eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        auto v0 = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }

        double d = pseudo::time_portion(
            env->env->reql_version(),
            v0->as_ptype(pseudo::time_string),
            component);
        return new_val(datum_t(d));
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
    time_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(4, 7)) { }
private:
    scoped_ptr_t<val_t> eval_impl(eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        rcheck(args->num_args() == 4 || args->num_args() == 7, base_exc_t::LOGIC,
               strprintf("Got %zu arguments to TIME (expected 4 or 7).",
                         args->num_args()));
        auto v0 = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }
        int year = v0->as_int<int>();

        auto v1 = args->arg(err_out, env, 1);
        if (err_out->has()) { return noval(); }
        int month = v1->as_int<int>();

        auto v2 = args->arg(err_out, env, 2);
        if (err_out->has()) { return noval(); }
        int day = v2->as_int<int>();
        int hours = 0;
        int minutes = 0;
        double seconds = 0;
        std::string tz;
        if (args->num_args() == 4) {
            auto v3 = args->arg(err_out, env, 3);
            if (err_out->has()) { return noval(); }
            tz = parse_tz(std::move(v3));
        } else if (args->num_args() == 7) {
            auto v3 = args->arg(err_out, env, 3);
            if (err_out->has()) { return noval(); }
            hours = v3->as_int<int>();

            auto v4 = args->arg(err_out, env, 4);
            if (err_out->has()) { return noval(); }
            minutes = v4->as_int<int>();

            auto v5 = args->arg(err_out, env, 5);
            if (err_out->has()) { return noval(); }
            seconds = v5->as_num();

            auto v6 = args->arg(err_out, env, 6);
            if (err_out->has()) { return noval(); }
            tz = parse_tz(std::move(v6));
        } else {
            r_sanity_check(false);
        }
        return new_val(
            pseudo::make_time(env->env->reql_version(), year, month, day, hours, minutes,
                              seconds, tz, this));
    }
    static std::string parse_tz(scoped_ptr_t<val_t> v) {
        return v->as_str().to_std();
    }
    virtual const char *name() const { return "time"; }
};

counted_t<term_t> make_iso8601_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<iso8601_term_t>(env, term);
}

counted_t<term_t> make_to_iso8601_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<to_iso8601_term_t>(env, term);
}

counted_t<term_t> make_epoch_time_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<epoch_time_term_t>(env, term);
}

counted_t<term_t> make_to_epoch_time_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<to_epoch_time_term_t>(env, term);
}

counted_t<term_t> make_now_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<now_term_t>(env, term);
}

counted_t<term_t> make_in_timezone_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<in_timezone_term_t>(env, term);
}

counted_t<term_t> make_during_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<during_term_t>(env, term);
}

counted_t<term_t> make_date_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<date_term_t>(env, term);
}

counted_t<term_t> make_time_of_day_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<time_of_day_term_t>(env, term);
}

counted_t<term_t> make_timezone_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<timezone_term_t>(env, term);
}

counted_t<term_t> make_time_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<time_term_t>(env, term);
}

counted_t<term_t> make_portion_term(
        compile_env_t *env, const raw_term_t &term,
        pseudo::time_component_t component) {
    return make_counted<portion_term_t>(env, term, component);
}

} // namespace ql

