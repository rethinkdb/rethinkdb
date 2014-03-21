// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>
#include <utility>
#include <vector>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"

namespace ql {

template<class T>
class map_acc_term_t : public grouped_seq_op_term_t {
protected:
    map_acc_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : grouped_seq_op_term_t(env, term, argspec_t(1, 2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, eval_flags_t) {
        return num_args() == 1
            ? arg(env, 0)->as_seq(env->env)->run_terminal(env->env, T(backtrace()))
            : arg(env, 0)->as_seq(env->env)->run_terminal(
                env->env, T(backtrace(), arg(env, 1)->as_func(GET_FIELD_SHORTCUT)));
    }
};

class sum_term_t : public map_acc_term_t<sum_wire_func_t> {
public:
    template<class... Args> sum_term_t(Args... args)
        : map_acc_term_t<sum_wire_func_t>(args...) { }
private:
    virtual const char *name() const { return "sum"; }
};
class avg_term_t : public map_acc_term_t<avg_wire_func_t> {
public:
    template<class... Args> avg_term_t(Args... args)
        : map_acc_term_t<avg_wire_func_t>(args...) { }
private:
    virtual const char *name() const { return "avg"; }
};
class min_term_t : public map_acc_term_t<min_wire_func_t> {
public:
    template<class... Args> min_term_t(Args... args)
        : map_acc_term_t<min_wire_func_t>(args...) { }
private:
    virtual const char *name() const { return "min"; }
};
class max_term_t : public map_acc_term_t<max_wire_func_t> {
public:
    template<class... Args> max_term_t(Args... args)
        : map_acc_term_t<max_wire_func_t>(args...) { }
private:
    virtual const char *name() const { return "max"; }
};

class count_term_t : public grouped_seq_op_term_t {
public:
    count_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : grouped_seq_op_term_t(env, term, argspec_t(1, 2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<val_t> v0 = arg(env, 0);
        if (num_args() == 1) {
            return v0->as_seq(env->env)
                ->run_terminal(env->env, count_wire_func_t());
        } else {
            counted_t<val_t> v1 = arg(env, 1);
            if (v1->get_type().is_convertible(val_t::type_t::FUNC)) {
                return v0->as_seq(env->env)
                    ->add_transformation(
                        env->env,
                        filter_wire_func_t(v1->as_func(), boost::none),
                        backtrace())
                    ->run_terminal(env->env, count_wire_func_t());
            } else {
                counted_t<func_t> f =
                    new_eq_comparison_func(v1->as_datum(), backtrace());
                return v0->as_seq(env->env)
                    ->add_transformation(
                        env->env, filter_wire_func_t(f, boost::none), backtrace())
                    ->run_terminal(env->env, count_wire_func_t());
            }
        }
    }
    virtual const char *name() const { return "count"; }
};

class map_term_t : public grouped_seq_op_term_t {
public:
    map_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : grouped_seq_op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return new_val(env->env, arg(env, 0)->as_seq(env->env)->add_transformation(
            env->env, map_wire_func_t(arg(env, 1)->as_func()), backtrace()));
    }
    virtual const char *name() const { return "map"; }
};

class concatmap_term_t : public grouped_seq_op_term_t {
public:
    concatmap_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : grouped_seq_op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return new_val(env->env, arg(env, 0)->as_seq(env->env)->add_transformation(
            env->env, concatmap_wire_func_t(arg(env, 1)->as_func()), backtrace()));
    }
    virtual const char *name() const { return "concatmap"; }
};

class group_term_t : public grouped_seq_op_term_t {
public:
    group_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : grouped_seq_op_term_t(env, term, argspec_t(1, -1), optargspec_t({"index", "multi"})) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        std::vector<counted_t<func_t> > funcs;
        funcs.reserve(num_args() - 1);
        for (size_t i = 1; i < num_args(); ++i) {
            funcs.push_back(arg(env, i)->as_func(GET_FIELD_SHORTCUT));
        }

        counted_t<datum_stream_t> seq;
        bool append_index = false;
        if (counted_t<val_t> index = optarg(env, "index")) {
            std::string index_str = index->as_str().to_std();
            counted_t<table_t> tbl = arg(env, 0)->as_table();
            if (index_str == tbl->get_pkey()) {
                auto field = make_counted<const datum_t>(std::move(index_str));
                funcs.push_back(new_get_field_func(field, backtrace()));
            } else {
                tbl->add_sorting(index_str, sorting_t::ASCENDING, this);
                append_index = true;
            }
            seq = tbl->as_datum_stream(env->env, backtrace());
        } else {
            seq = arg(env, 0)->as_seq(env->env);
        }

        rcheck((funcs.size() + append_index) != 0, base_exc_t::GENERIC,
               "Cannot group by nothing.");

        bool multi = false;
        if (counted_t<val_t> multi_val = optarg(env, "multi")) {
            multi = multi_val->as_bool();
        }

        seq = seq->add_grouping(
            env->env,
            group_wire_func_t(std::move(funcs), append_index, multi), backtrace());

        return new_val(env->env, seq);
    }
    virtual const char *name() const { return "group"; }
};

class filter_term_t : public grouped_seq_op_term_t {
public:
    filter_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : grouped_seq_op_term_t(env, term, argspec_t(2), optargspec_t({"default"})),
          default_filter_term(lazy_literal_optarg(env, "default")) { }

private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<val_t> v0 = arg(env, 0);
        counted_t<val_t> v1 = arg(env, 1, LITERAL_OK);
        counted_t<func_t> f = v1->as_func(CONSTANT_SHORTCUT);
        boost::optional<wire_func_t> defval;
        if (default_filter_term.has()) {
            defval = wire_func_t(default_filter_term->eval_to_func(env->scope));
        }

        if (v0->get_type().is_convertible(val_t::type_t::SELECTION)) {
            std::pair<counted_t<table_t>, counted_t<datum_stream_t> > ts
                = v0->as_selection(env->env);
            return new_val(ts.second->add_transformation(
                               env->env, filter_wire_func_t(f, defval), backtrace()),
                           ts.first);
        } else {
            return new_val(env->env, v0->as_seq(env->env)->add_transformation(
                               env->env, filter_wire_func_t(f, defval), backtrace()));
        }
    }

    virtual const char *name() const { return "filter"; }

    counted_t<func_term_t> default_filter_term;
};

class reduce_term_t : public grouped_seq_op_term_t {
public:
    reduce_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        grouped_seq_op_term_t(env, term, argspec_t(2), optargspec_t({ "base" })) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return arg(env, 0)->as_seq(env->env)->run_terminal(
            env->env, reduce_wire_func_t(arg(env, 1)->as_func()));
    }
    virtual const char *name() const { return "reduce"; }
};

// TODO: this sucks.  Change to use the same macros as rewrites.hpp?
class between_term_t : public bounded_op_term_t {
public:
    between_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : bounded_op_term_t(env, term, argspec_t(3), optargspec_t({"index"})) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        counted_t<table_t> tbl = arg(env, 0)->as_table();
        counted_t<const datum_t> lb = arg(env, 1)->as_datum();
        if (lb->get_type() == datum_t::R_NULL) {
            lb.reset();
        }
        counted_t<const datum_t> rb = arg(env, 2)->as_datum();
        if (rb->get_type() == datum_t::R_NULL) {
            rb.reset();
        }

        if (lb.has() && rb.has()) {
            if (*lb > *rb || ((left_open(env) || right_open(env)) && *lb == *rb)) {
                counted_t<const datum_t> arr = make_counted<datum_t>(datum_t::R_ARRAY);
                counted_t<datum_stream_t> ds(
                    new array_datum_stream_t(arr, backtrace()));
                return new_val(ds, tbl);
            }
        }

        counted_t<val_t> sindex = optarg(env, "index");
        std::string sid = (sindex.has() ? sindex->as_str().to_std() : tbl->get_pkey());

        tbl->add_bounds(
            datum_range_t(
                lb, left_open(env) ? key_range_t::open : key_range_t::closed,
                rb, right_open(env) ? key_range_t::open : key_range_t::closed),
            sid, this);
        return new_val(tbl);
    }
    virtual const char *name() const { return "between"; }

    protob_t<Term> filter_func;
};

class union_term_t : public op_term_t {
public:
    union_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(0, -1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        std::vector<counted_t<datum_stream_t> > streams;
        for (size_t i = 0; i < num_args(); ++i) {
            streams.push_back(arg(env, i)->as_seq(env->env));
        }
        counted_t<datum_stream_t> union_stream
            = make_counted<union_datum_stream_t>(std::move(streams), backtrace());
        return new_val(env->env, union_stream);
    }
    virtual const char *name() const { return "union"; }
};

class zip_term_t : public op_term_t {
public:
    zip_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, UNUSED eval_flags_t flags) {
        return new_val(env->env, arg(env, 0)->as_seq(env->env)->zip());
    }
    virtual const char *name() const { return "zip"; }
};

counted_t<term_t> make_between_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<between_term_t>(env, term);
}
counted_t<term_t> make_reduce_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<reduce_term_t>(env, term);
}
counted_t<term_t> make_map_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<map_term_t>(env, term);
}
counted_t<term_t> make_filter_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<filter_term_t>(env, term);
}
counted_t<term_t> make_concatmap_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<concatmap_term_t>(env, term);
}
counted_t<term_t> make_group_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<group_term_t>(env, term);
}
counted_t<term_t> make_count_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<count_term_t>(env, term);
}
counted_t<term_t> make_avg_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<avg_term_t>(env, term);
}
counted_t<term_t> make_sum_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<sum_term_t>(env, term);
}
counted_t<term_t> make_min_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<min_term_t>(env, term);
}
counted_t<term_t> make_max_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<max_term_t>(env, term);
}
counted_t<term_t> make_union_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<union_term_t>(env, term);
}
counted_t<term_t> make_zip_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<zip_term_t>(env, term);
}

} // namespace ql
