// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>
#include <utility>
#include <vector>

#include "parsing/utf8.hpp"
#include "rdb_protocol/datum_stream/eq_join.hpp"
#include "rdb_protocol/datum_stream/fold.hpp"
#include "rdb_protocol/datum_stream/map.hpp"
#include "rdb_protocol/datum_stream/ordered_union.hpp"
#include "rdb_protocol/datum_stream/range.hpp"
#include "rdb_protocol/datum_stream/union.hpp"
#include "rdb_protocol/datum_stream/vector.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/math_utils.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/order_util.hpp"

namespace ql {

template<class T>
class map_acc_term_t : public grouped_seq_op_term_t {
protected:
    map_acc_term_t(compile_env_t *env, const raw_term_t &term)
        : grouped_seq_op_term_t(env, term, argspec_t(1, 2), optargspec_t({"index"})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args,
                                       eval_flags_t) const {
        scoped_ptr_t<val_t> v = args->arg(env, 0);
        scoped_ptr_t<val_t> idx = args->optarg(env, "index");
        counted_t<const func_t> func;
        if (args->num_args() == 2) {
            func = args->arg(env, 1)->as_func(GET_FIELD_SHORTCUT);
        }
        if (!func.has() && !idx.has()) {
            // TODO: make this use a table slice.
            if (uses_idx() && v->get_type().is_convertible(val_t::type_t::TABLE)) {
                return on_idx(env->env, v->as_table(), std::move(idx));
            } else {
                return v->as_seq(env->env)->run_terminal(env->env, T(backtrace()));
            }
        } else if (func.has() && !idx.has()) {
            return v->as_seq(env->env)->run_terminal(env->env, T(backtrace(), func));
        } else if (!func.has() && idx.has()) {
            return on_idx(env->env, v->as_table(), std::move(idx));
        } else {
            rfail(base_exc_t::LOGIC,
                  "Cannot provide both a function and an index to %s.",
                  name());
        }
    }
    virtual bool uses_idx() const = 0;
    virtual scoped_ptr_t<val_t> on_idx(
        env_t *env, counted_t<table_t> tbl, scoped_ptr_t<val_t> idx) const = 0;
};

template<class T>
class unindexable_map_acc_term_t : public map_acc_term_t<T> {
protected:
    template<class... Args> unindexable_map_acc_term_t(Args... args)
        : map_acc_term_t<T>(args...) { }
private:
    virtual bool uses_idx() const { return false; }
    virtual scoped_ptr_t<val_t> on_idx(
        env_t *, counted_t<table_t>, scoped_ptr_t<val_t>) const {
        rfail(base_exc_t::LOGIC, "Cannot call %s on an index.", this->name());
    }
};
class sum_term_t : public unindexable_map_acc_term_t<sum_wire_func_t> {
public:
    template<class... Args> sum_term_t(Args... args)
        : unindexable_map_acc_term_t<sum_wire_func_t>(args...) { }
private:
    virtual const char *name() const { return "sum"; }
};
class avg_term_t : public unindexable_map_acc_term_t<avg_wire_func_t> {
public:
    template<class... Args> avg_term_t(Args... args)
        : unindexable_map_acc_term_t<avg_wire_func_t>(args...) { }
private:
    virtual const char *name() const { return "avg"; }
};

template<class T>
class indexable_map_acc_term_t : public map_acc_term_t<T> {
protected:
    template<class... Args> indexable_map_acc_term_t(Args... args)
        : map_acc_term_t<T>(args...) { }
    virtual ~indexable_map_acc_term_t() { }
private:
    virtual bool uses_idx() const { return true; }
    virtual scoped_ptr_t<val_t> on_idx(
        env_t *env, counted_t<table_t> tbl, scoped_ptr_t<val_t> idx) const {
        optional<std::string> idx_str;
        if (idx.has()) {
            idx_str.set(idx->as_str().to_std());
        }
        counted_t<table_slice_t> slice(
            new table_slice_t(tbl, idx_str, sorting()));
        return term_t::new_val(single_selection_t::from_slice(
            env,
            term_t::backtrace(),
            slice,
            strprintf("`%s` found no entries in the specified index.",
                      this->name())));
    }
    virtual sorting_t sorting() const = 0;
};

class min_term_t : public indexable_map_acc_term_t<min_wire_func_t> {
public:
    template<class... Args> min_term_t(Args... args)
        : indexable_map_acc_term_t<min_wire_func_t>(args...) { }
private:
    virtual sorting_t sorting() const { return sorting_t::ASCENDING; }
    virtual const char *name() const { return "min"; }
};
class max_term_t : public indexable_map_acc_term_t<max_wire_func_t> {
public:
    template<class... Args> max_term_t(Args... args)
        : indexable_map_acc_term_t<max_wire_func_t>(args...) { }
private:
    virtual sorting_t sorting() const { return sorting_t::DESCENDING; }
    virtual const char *name() const { return "max"; }
};

class count_term_t : public grouped_seq_op_term_t {
public:
    count_term_t(compile_env_t *env, const raw_term_t &term)
        : grouped_seq_op_term_t(env, term, argspec_t(1, 2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args,
                                          eval_flags_t) const {
        scoped_ptr_t<val_t> v0 = args->arg(env, 0);
        if (args->num_args() == 1) {
            if (v0->get_type().is_convertible(val_t::type_t::DATUM)) {
                datum_t d = v0->as_datum();
                switch (static_cast<int>(d.get_type())) { // TODO: See issue 5177
                case datum_t::R_BINARY:
                    return new_val(datum_t(
                       safe_to_double(d.as_binary().size())));
                case datum_t::R_STR:
                    return new_val(datum_t(
                        safe_to_double(utf8::count_codepoints(d.as_str()))));
                case datum_t::R_OBJECT:
                    return new_val(datum_t(
                        safe_to_double(d.obj_size())));
                default:
                    break;
                }
            }
            return v0->as_seq(env->env)
                ->run_terminal(env->env, count_wire_func_t());
        } else {
            scoped_ptr_t<val_t> v1 = args->arg(env, 1);
            if (v1->get_type().is_convertible(val_t::type_t::FUNC)) {
                counted_t<datum_stream_t> stream = v0->as_seq(env->env);
                stream->add_transformation(
                        filter_wire_func_t(v1->as_func(), r_nullopt),
                        backtrace());
                return stream->run_terminal(env->env, count_wire_func_t());
            } else {
                counted_t<const func_t> f =
                    new_eq_comparison_func(v1->as_datum(), backtrace());
                counted_t<datum_stream_t> stream = v0->as_seq(env->env);
                stream->add_transformation(
                        filter_wire_func_t(f, r_nullopt), backtrace());

                return stream->run_terminal(env->env, count_wire_func_t());
            }
        }
    }
    virtual const char *name() const { return "count"; }
};

class map_term_t : public grouped_seq_op_term_t {
public:
    map_term_t(compile_env_t *env, const raw_term_t &term)
        : grouped_seq_op_term_t(env, term, argspec_t(2, -1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env,
                                          args_t *args,
                                          eval_flags_t) const {
        std::vector<counted_t<datum_stream_t> > streams;
        streams.reserve(args->num_args() - 1);
        for (size_t i = 0; i < args->num_args() - 1; ++i) {
            streams.push_back(args->arg(env, i)->as_seq(env->env));
        }

        counted_t<const func_t> func =
            args->arg(env, args->num_args() - 1)->as_func();
        optional<std::size_t> func_arity = func->arity();
        if (!!func_arity) {
            rcheck(func_arity.get() == 0 || func_arity.get() == args->num_args() - 1,
                   base_exc_t::LOGIC,
                   strprintf("The function passed to `map` expects %zu argument%s, "
                             "but %zu sequence%s found.",
                             func_arity.get(),
                             (func_arity.get() == 1 ? "" : "s"),
                             args->num_args() - 1,
                             (args->num_args() == 2 ? " was" : "s were")));
        }

        if (args->num_args() == 2) {
            streams.front()->add_transformation(
                    map_wire_func_t(std::move(func)), backtrace());
            return new_val(env->env, streams.front());
        } else {
            counted_t<datum_stream_t> map_stream = make_counted<map_datum_stream_t>(
                std::move(streams), std::move(func), backtrace());
            return new_val(env->env, map_stream);
        }
    }
    virtual const char *name() const { return "map"; }
};

class eq_join_term_t : public grouped_seq_op_term_t {
public:
    eq_join_term_t(compile_env_t *env, const raw_term_t &term)
        : grouped_seq_op_term_t(env,
                                term,
                                argspec_t(3),
                                optargspec_t({"index", "ordered"})) { }

    virtual const char *name() const { return "eqjoin"; }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env,
                                          args_t *args,
                                          eval_flags_t) const {
        counted_t<datum_stream_t> stream = args->arg(env, 0)->as_seq(env->env);
        counted_t<table_t> table = args->arg(env, 2)->as_table();

        // Either a field name or a predicate function:
        counted_t<const func_t> predicate_function;

        predicate_function = args->arg(env, 1)->as_func(GET_FIELD_SHORTCUT);

        bool ordered = false;
        scoped_ptr_t<val_t> maybe_ordered = args->optarg(env, "ordered");
        if (maybe_ordered.has()) {
            ordered = maybe_ordered->as_bool();
        }
        datum_t key;
        scoped_ptr_t<val_t> maybe_key = args->optarg(env, "index");
        if (maybe_key.has()) {
            key = maybe_key->as_datum();
        } else {
            key = datum_t(datum_string_t(table->get_pkey()));
        }
        counted_t<eq_join_datum_stream_t> eq_join_stream =
            make_counted<eq_join_datum_stream_t>(stream,
                                                 table,
                                                 key.as_str(),
                                                 predicate_function,
                                                 ordered,
                                                 backtrace());

        return new_val(env->env, eq_join_stream);
    }
};

class fold_term_t : public grouped_seq_op_term_t {
public:
    fold_term_t(compile_env_t *env, const raw_term_t &term)
      : grouped_seq_op_term_t(env,
                              term,
                              argspec_t(3),
                              optargspec_t({"emit", "final_emit"})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env,
                                          args_t *args,
                                          eval_flags_t) const {
        counted_t<datum_stream_t> stream = args->arg(env, 0)->as_seq(env->env);

        datum_t base = args->arg(env, 1)->as_datum();

        counted_t<const func_t> acc_func =
            args->arg(env, 2)->as_func();
        optional<std::size_t> acc_func_arity = acc_func->arity();

        if (static_cast<bool>(acc_func_arity)) {
            rcheck(acc_func_arity.get() == 0 || acc_func_arity.get() == 2,
                   base_exc_t::LOGIC,
                   strprintf("The accumulator function passed to `fold`"
                             " should expect 2 arguments"));
        }

        scoped_ptr_t<val_t> emit_arg = args->optarg(env, "emit");
        scoped_ptr_t<val_t> final_emit_arg = args->optarg(env, "final_emit");

        if (!emit_arg.has()) {
            // Handle case without emit function.
            datum_t result = base;
            batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env->env);
            {
                datum_t row;
                std::vector<datum_t> acc_args;
                while (row = stream->next(env->env, batchspec), row.has()) {
                    acc_args.push_back(std::move(result));
                    acc_args.push_back(std::move(row));

                    result = acc_func->call(env->env, acc_args)->as_datum();

                    r_sanity_check(result.has());
                    acc_args.clear();
                }
            }

            if (final_emit_arg.has()) {
                datum_t final_result;
                std::vector<datum_t> final_args{std::move(result)};

                counted_t<const func_t> final_emit_func = final_emit_arg->as_func();
                final_result = final_emit_func->call(env->env, final_args)->as_datum();
                r_sanity_check(final_result.has());
                return new_val(final_result);
            } else {
                return new_val(result);
            }
        } else {
            counted_t<const func_t> emit_func = emit_arg->as_func();
            counted_t<datum_stream_t> fold_stream;
            if (final_emit_arg.has()) {
                counted_t<const func_t> final_emit_func = final_emit_arg->as_func();
                fold_stream
                    = make_counted<fold_datum_stream_t>(std::move(stream),
                                                        base,
                                                        std::move(acc_func),
                                                        std::move(emit_func),
                                                        std::move(final_emit_func),
                                                        backtrace());
            } else {
                fold_stream
                    = make_counted<fold_datum_stream_t>(std::move(stream),
                                                        base,
                                                        std::move(acc_func),
                                                        std::move(emit_func),
                                                        counted_t<const func_t>(),
                                                        backtrace());
            }
            return new_val(env->env, fold_stream);
        }
    }
    virtual const char *name() const { return "fold"; }
};

class concatmap_term_t : public grouped_seq_op_term_t {
public:
    concatmap_term_t(compile_env_t *env, const raw_term_t &term)
        : grouped_seq_op_term_t(env, term, argspec_t(2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<datum_stream_t> stream = args->arg(env, 0)->as_seq(env->env);
        stream->add_transformation(
            concatmap_wire_func_t(result_hint_t::NO_HINT,
                                  args->arg(env, 1)->as_func()),
                backtrace());
        return new_val(env->env, stream);
    }
    virtual const char *name() const { return "concatmap"; }
};

class group_term_t : public grouped_seq_op_term_t {
public:
    group_term_t(compile_env_t *env, const raw_term_t &term)
        : grouped_seq_op_term_t(env, term, argspec_t(1, -1),
                                optargspec_t({"index", "multi"})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        std::vector<counted_t<const func_t> > funcs;
        funcs.reserve(args->num_args() - 1);
        for (size_t i = 1; i < args->num_args(); ++i) {
            funcs.push_back(args->arg(env, i)->as_func(GET_FIELD_SHORTCUT));
        }

        counted_t<datum_stream_t> seq;
        bool append_index = false;
        if (scoped_ptr_t<val_t> index = args->optarg(env, "index")) {
            std::string index_str = index->as_str().to_std();
            counted_t<table_t> tbl = args->arg(env, 0)->as_table();
            counted_t<table_slice_t> slice;
            if (index_str == tbl->get_pkey()) {
                auto field = index->as_datum();
                funcs.push_back(new_get_field_func(field, backtrace()));
                slice = make_counted<table_slice_t>(tbl, make_optional(index_str));
            } else {
                slice = make_counted<table_slice_t>(
                    tbl, make_optional(index_str), sorting_t::ASCENDING);
                append_index = true;
            }
            r_sanity_check(slice.has());
            seq = slice->as_seq(env->env, backtrace());
        } else {
            seq = args->arg(env, 0)->as_seq(env->env);
        }

        rcheck((funcs.size() + append_index) != 0, base_exc_t::LOGIC,
               "Cannot group by nothing.");

        bool multi = false;
        if (scoped_ptr_t<val_t> multi_val = args->optarg(env, "multi")) {
            multi = multi_val->as_bool();
        }

        seq->add_grouping(group_wire_func_t(std::move(funcs),
                                            append_index,
                                            multi),
                          backtrace());

        return new_val(env->env, seq);
    }
    virtual const char *name() const { return "group"; }
};

class filter_term_t : public grouped_seq_op_term_t {
public:
    filter_term_t(compile_env_t *env, const raw_term_t &term)
        : grouped_seq_op_term_t(env, term, argspec_t(2), optargspec_t({"default"})),
          default_filter_term(lazy_literal_optarg(env, "default")) { }

private:
    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v0 = args->arg(env, 0);
        scoped_ptr_t<val_t> v1 = args->arg(env, 1, LITERAL_OK);
        counted_t<const func_t> f = v1->as_func(CONSTANT_SHORTCUT);
        optional<wire_func_t> defval;
        if (default_filter_term.has()) {
            defval.set(wire_func_t(default_filter_term->eval_to_func(env->scope)));
        }

        if (v0->get_type().is_convertible(val_t::type_t::SELECTION)) {
            counted_t<selection_t> ts = v0->as_selection(env->env);
            ts->seq->add_transformation(filter_wire_func_t(f, defval), backtrace());
            return new_val(ts);
        } else {
            counted_t<datum_stream_t> stream = v0->as_seq(env->env);
            stream->add_transformation(
                    filter_wire_func_t(f, defval), backtrace());
            return new_val(env->env, stream);
        }
    }

    virtual const char *name() const { return "filter"; }

    counted_t<const func_term_t> default_filter_term;
};

class reduce_term_t : public grouped_seq_op_term_t {
public:
    reduce_term_t(compile_env_t *env, const raw_term_t &term)
        : grouped_seq_op_term_t(env, term, argspec_t(2),
                                optargspec_t({ "base" })) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {
        return args->arg(env, 0)->as_seq(env->env)->run_terminal(
            env->env, reduce_wire_func_t(args->arg(env, 1)->as_func()));
    }
    virtual const char *name() const { return "reduce"; }
};

struct rcheck_transform_visitor_t : public bt_rcheckable_t,
                                    public boost::static_visitor<void> {
    explicit rcheck_transform_visitor_t(backtrace_id_t bt)
        : bt_rcheckable_t(bt) { }
    void check_f(const wire_func_t &f) const {
        if (!f.compile_wire_func()->is_deterministic().test(single_server_t::yes,
                                                            constant_now_t::yes)) {
            rfail_src(f.get_bt(),
                      base_exc_t::LOGIC,
                      "Cannot call `changes` after a non-deterministic function.");
        }
    }
    void operator()(const map_wire_func_t &f) const {
        check_f(f);
    }
    void operator()(const filter_wire_func_t &f) const {
        check_f(f.filter_func);
        if (f.default_filter_val.has_value()) {
            check_f(*f.default_filter_val);
        }
    }
    void operator()(const concatmap_wire_func_t &f) const {
        switch (f.result_hint) {
        case result_hint_t::AT_MOST_ONE:
            check_f(f);
            break;
        case result_hint_t::NO_HINT:
            rfail(base_exc_t::LOGIC, "Cannot call `changes` after `concat_map`.");
            // fallthru

        default: unreachable();
        }
    }
    NORETURN void operator()(const group_wire_func_t &) const {
        rfail(base_exc_t::LOGIC, "Cannot call `changes` after `group`.");
    }
    NORETURN void operator()(const distinct_wire_func_t &) const {
        rfail(base_exc_t::LOGIC, "Cannot call `changes` after `distinct`.");
    }
    NORETURN void operator()(const zip_wire_func_t &) const {
        rfail(base_exc_t::LOGIC, "Cannot call `changes` after `zip`.");
    }
};

struct rcheck_spec_visitor_t : public bt_rcheckable_t,
                               public boost::static_visitor<void> {
    explicit rcheck_spec_visitor_t(env_t *_env, backtrace_id_t bt)
        : bt_rcheckable_t(bt), env(_env) { }
    void operator()(const changefeed::keyspec_t::range_t &spec) const {
        for (const auto &t : spec.transforms) {
            boost::apply_visitor(rcheck_transform_visitor_t(backtrace()), t);
        }
    }
    void operator()(const changefeed::keyspec_t::limit_t &spec) const {
        (*this)(spec.range);
        rcheck(spec.limit <= env->limits().array_size_limit(), base_exc_t::RESOURCE,
               strprintf(
                   "Array size limit `%zu` exceeded.  (`.limit(X).changes()` is illegal "
                   "if X is larger than the array size limit.)",
                   env->limits().array_size_limit()));
    }
    void operator()(const changefeed::keyspec_t::point_t &) const { }
    void operator()(const changefeed::keyspec_t::empty_t &) const { }
    env_t *env;
};

class changes_term_t : public op_term_t {
public:
    changes_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(
            env, term, argspec_t(1),
            optargspec_t({"squash",
                          "changefeed_queue_size",
                          "include_initial",
                          "include_offsets",
                          "include_states",
                          "include_types"})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {

        scoped_ptr_t<val_t> sval = args->optarg(env, "squash");
        datum_t squash = sval.has() ? sval->as_datum() : datum_t::boolean(false);
        if (squash.get_type() == datum_t::type_t::R_NUM) {
            rcheck_target(sval, squash.as_num() >= 0.0, base_exc_t::LOGIC,
                          "Expected BOOL or a positive NUMBER but found "
                          "a negative NUMBER.");
        } else if (squash.get_type() != datum_t::type_t::R_BOOL) {
            rfail_target(sval, base_exc_t::LOGIC,
                         "Expected BOOL or NUMBER but found %s.",
                         squash.get_type_name().c_str());
        }

        bool include_states = false;
        if (scoped_ptr_t<val_t> v = args->optarg(env, "include_states")) {
            include_states = v->as_bool();
        }

        bool include_types = false;
        if (scoped_ptr_t<val_t> v = args->optarg(env, "include_types")) {
            include_types = v->as_bool();
        }

        bool include_initial = false;
        if (scoped_ptr_t<val_t> v = args->optarg(env, "include_initial")) {
            include_initial = v->as_bool();
        }

        bool include_offsets = false;
        if (scoped_ptr_t<val_t> v = args->optarg(env, "include_offsets")) {
            include_offsets = v->as_bool();
        }

        scoped_ptr_t<val_t> v = args->arg(env, 0);
        configured_limits_t limits = env->env->limits_with_changefeed_queue_size(
                args->optarg(env, "changefeed_queue_size"));
        if (v->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
            counted_t<datum_stream_t> seq = v->as_seq(env->env);
            std::vector<counted_t<datum_stream_t> > streams;
            std::vector<changespec_t> changespecs = seq->get_changespecs();
            r_sanity_check(changespecs.size() >= 1);
            for (auto &&changespec : changespecs) {
                if (include_initial) {
                    r_sanity_check(changespec.stream.has());
                }
                boost::apply_visitor(rcheck_spec_visitor_t(env->env, backtrace()),
                                     changespec.keyspec.spec);
                streams.push_back(
                    changespec.keyspec.table->read_changes(
                        env->env,
                        changefeed::streamspec_t(
                            include_initial
                                ? std::move(changespec.stream)
                                : counted_t<datum_stream_t>(),
                            changespec.keyspec.table_name,
                            include_offsets,
                            include_states,
                            include_types,
                            limits,
                            squash,
                            std::move(changespec.keyspec.spec)),
                        backtrace()));
            }
            if (streams.size() == 1) {
                return new_val(env->env, streams[0]);
            } else {
                return new_val(
                    env->env,
                    make_counted<union_datum_stream_t>(
                        env->env,
                        std::move(streams),
                        backtrace(),
                        streams.size()));
            }
        } else if (v->get_type().is_convertible(val_t::type_t::SINGLE_SELECTION)) {
            auto sel = v->as_single_selection();
            return new_val(
                env->env,
                sel->get_tbl()->tbl->read_changes(
                    env->env,
                    changefeed::streamspec_t(
                        include_initial
                            // We want to provide an empty stream in this case
                            // because we get the initial values from the stamp
                            // read instead.
                            ? make_counted<vector_datum_stream_t>(
                                sel->get_bt(), std::vector<datum_t>(), r_nullopt)
                            : counted_t<vector_datum_stream_t>(),
                        sel->get_tbl()->display_name(),
                        include_offsets,
                        include_states,
                        include_types,
                        limits,
                        squash,
                        sel->get_spec()),
                    sel->get_bt()));
        }
        auto selection = v->as_selection(env->env);
        rfail(base_exc_t::LOGIC,
              ".changes() not yet supported on range selections");
    }
    virtual const char *name() const { return "changes"; }
};

class minval_term_t final : public op_term_t {
public:
    minval_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(0)) { }
private:
    scoped_ptr_t<val_t> eval_impl(scope_env_t *, args_t *, eval_flags_t) const {
        return new_val(datum_t::minval());
    }
    const char *name() const { return "minval"; }
};

class maxval_term_t final : public op_term_t {
public:
    maxval_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(0)) { }
private:
    scoped_ptr_t<val_t> eval_impl(scope_env_t *, args_t *, eval_flags_t) const {
        return new_val(datum_t::maxval());
    }
    const char *name() const { return "maxval"; }
};

// For compatibility, the old version of `between` treats `null` as unbounded, and the
// new version of between will error on a `null` boundary (r.minval and r.maxval should
// be used instead).  The deprecated version can be removed in a few versions, and the
// new version can remove the error once we support `null` in indexes.
enum class between_null_t { UNBOUNDED, ERROR };

class between_term_t : public bounded_op_term_t {
public:
    between_term_t(compile_env_t *env, const raw_term_t &term,
                   between_null_t _null_behavior)
        : bounded_op_term_t(env, term, argspec_t(3), optargspec_t({"index"})),
          null_behavior(_null_behavior) { }
private:
    datum_t check_bound(scoped_ptr_t<val_t> bound_val,
                        datum_t::type_t unbounded_type) const {
        datum_t bound = bound_val->as_datum();
        if (bound.get_type() == datum_t::R_NULL) {
            rcheck_target(bound_val, null_behavior != between_null_t::ERROR,
                          base_exc_t::LOGIC,
                          "Cannot use `null` in BETWEEN, use `r.minval` or `r.maxval` "
                          "to denote unboundedness.");
            bound = unbounded_type == datum_t::type_t::MINVAL ? datum_t::minval() :
                                                                datum_t::maxval();
        }
        return bound;
    }

    virtual scoped_ptr_t<val_t>
    eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_slice_t> tbl_slice = args->arg(env, 0)->as_table_slice();
        bool left_open = is_left_open(env, args);
        bool right_open = is_right_open(env, args);
        datum_t lb = check_bound(args->arg(env, 1), datum_t::type_t::MINVAL);
        datum_t rb = check_bound(args->arg(env, 2), datum_t::type_t::MAXVAL);

        scoped_ptr_t<val_t> sindex = args->optarg(env, "index");
        std::string idx;
        if (sindex.has()) {
            idx = sindex->as_str().to_std();
        } else {
            optional<std::string> old_idx = tbl_slice->get_idx();
            idx = old_idx ? *old_idx : tbl_slice->get_tbl()->get_pkey();
        }
        return new_val(
            // `table_slice_t` can handle emtpy / invalid `datum_range_t`'s, checking is
            // done there.
            tbl_slice->with_bounds(
                idx,
                datum_range_t(
                    lb, left_open ? key_range_t::open : key_range_t::closed,
                    rb, right_open ? key_range_t::open : key_range_t::closed)));
    }
    virtual const char *name() const { return "between"; }

    between_null_t null_behavior;
};


class union_term_t : public op_term_t {
public:
    union_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(0, -1), optargspec_t({"interleave"})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env,
                                          args_t *args,
                                          eval_flags_t eval_flags) const {
        std::vector<counted_t<datum_stream_t> > streams;
        for (size_t i = 0; i < args->num_args(); ++i) {
            streams.push_back(args->arg(env, i)->as_seq(env->env));
        }

        optional<raw_term_t> r_interleave_arg_op = get_src().optarg("interleave");

        std::vector<scoped_ptr_t<val_t> > evaluated_interleave_args;

        bool allow_unordered_interleave = true;
        bool order_by_field = false;

        counted_t<datum_stream_t> union_stream;

        if (r_interleave_arg_op) {

            if (r_interleave_arg_op->type() == Term::MAKE_ARRAY) {
                // Array of elements which may contain functions or r.desc/asc.
                allow_unordered_interleave = false;
                order_by_field = true;

                counted_t<const term_t> interleave_term;
                // Steal arguments from an array as an optarg to allow functions in array.
                auto it = optargs.find("interleave");
                r_sanity_check(it != optargs.end());
                interleave_term = it->second;

                const std::vector<counted_t<const term_t> > &array_args
                    = interleave_term->get_original_args();

                std::vector<scoped_ptr_t<val_t> > array_args_evaluated;
                for (auto &arg : array_args) {
                    array_args_evaluated.push_back(arg->eval(env, eval_flags));
                }

                union_stream = make_counted<ordered_union_datum_stream_t>(
                    std::move(streams),
                    build_comparisons_from_optional_terms(this,
                                                          env,
                                                          std::move(array_args_evaluated),
                                                          *r_interleave_arg_op),
                    env->env,
                    backtrace());
            } else {
                scoped_ptr_t<val_t> interleave_arg = args->optarg(env, "interleave");
                // A single element, either a bool or a term as above.
                datum_t interleave_datum;
                bool use_as_term = true;
                if (interleave_arg->get_type().is_convertible(val_t::type_t::DATUM)) {
                    interleave_datum = interleave_arg->as_datum();
                    if (interleave_datum.get_type() == datum_t::type_t::R_BOOL) {
                        order_by_field = false;
                        allow_unordered_interleave = interleave_datum.as_bool();
                        use_as_term = false;
                    }
                }
                if (use_as_term) {
                    allow_unordered_interleave = false;
                    order_by_field = true;

                    union_stream = make_counted<ordered_union_datum_stream_t>(
                        std::move(streams),
                        build_comparisons_from_single_term(this,
                                                           env,
                                                           std::move(interleave_arg),
                                                           *r_interleave_arg_op),
                        env->env,
                        backtrace());
                }
            }
        }

        if (allow_unordered_interleave) {
            union_stream = make_counted<union_datum_stream_t>(
                env->env, std::move(streams), backtrace());
        } else if (!order_by_field) {
            union_stream = make_counted<ordered_union_datum_stream_t>(
                std::move(streams),
                std::vector<std::pair<order_direction_t, counted_t<const func_t> > >(),
                env->env,
                backtrace());
        }
        r_sanity_check(union_stream.has());
        return new_val(env->env, union_stream);
    }
    virtual const char *name() const { return "union"; }
};

class zip_term_t : public op_term_t {
public:
    zip_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<datum_stream_t> stream = args->arg(env, 0)->as_seq(env->env);
        stream->add_transformation(zip_wire_func_t(), backtrace());
        return new_val(env->env, stream);
    }
    virtual const char *name() const { return "zip"; }
};

class range_term_t : public op_term_t {
public:
    range_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(0, 2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        bool is_infinite = false;
        int64_t start = 0, stop = 0;

        if (args->num_args() == 0) {
            is_infinite = true;
        } else if (args->num_args() == 1) {
            stop = args->arg(env, 0)->as_int();
        } else {
            r_sanity_check(args->num_args() == 2);
            start = args->arg(env, 0)->as_int();
            stop = args->arg(env, 1)->as_int();
        }

        return new_val(env->env, make_counted<range_datum_stream_t>(
            is_infinite, start, stop, backtrace()));
    }
    virtual const char *name() const { return "range"; }
};

counted_t<term_t> make_minval_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<minval_term_t>(env, term);
}

counted_t<term_t> make_maxval_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<maxval_term_t>(env, term);
}

counted_t<term_t> make_between_deprecated_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<between_term_t>(env, term, between_null_t::UNBOUNDED);
}

counted_t<term_t> make_between_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<between_term_t>(env, term, between_null_t::ERROR);
}

counted_t<term_t> make_changes_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<changes_term_t>(env, term);
}

counted_t<term_t> make_reduce_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<reduce_term_t>(env, term);
}

counted_t<term_t> make_map_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<map_term_t>(env, term);
}

counted_t<term_t> make_eq_join_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<eq_join_term_t>(env, term);
}

counted_t<term_t> make_fold_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<fold_term_t>(env, term);
}
counted_t<term_t> make_filter_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<filter_term_t>(env, term);
}

counted_t<term_t> make_concatmap_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<concatmap_term_t>(env, term);
}

counted_t<term_t> make_group_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<group_term_t>(env, term);
}

counted_t<term_t> make_count_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<count_term_t>(env, term);
}

counted_t<term_t> make_avg_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<avg_term_t>(env, term);
}

counted_t<term_t> make_sum_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<sum_term_t>(env, term);
}

counted_t<term_t> make_min_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<min_term_t>(env, term);
}

counted_t<term_t> make_max_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<max_term_t>(env, term);
}

counted_t<term_t> make_union_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<union_term_t>(env, term);
}

counted_t<term_t> make_zip_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<zip_term_t>(env, term);
}

counted_t<term_t> make_range_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<range_term_t>(env, term);
}

} // namespace ql
