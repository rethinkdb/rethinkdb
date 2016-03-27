// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/arr.hpp"

#include "math.hpp"
#include "parsing/utf8.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "stl_utils.hpp"

#include "debug.hpp"

namespace ql {

class pend_term_t : public op_term_t {
public:
    pend_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
protected:
    enum which_pend_t { PRE, AP };

    scoped_ptr_t<val_t> pend(scope_env_t *env, args_t *args, which_pend_t which_pend) const {
        datum_t arr = args->arg(env, 0)->as_datum();
        datum_t new_el = args->arg(env, 1)->as_datum();
        datum_array_builder_t out(env->env->limits());
        out.reserve(arr.arr_size() + 1);
        if (which_pend == PRE) {
            // TODO: this is horrendously inefficient.
            out.add(new_el);
            for (size_t i = 0; i < arr.arr_size(); ++i) {
                out.add(arr.get(i));
            }
        } else {
            // TODO: this is horrendously inefficient.
            for (size_t i = 0; i < arr.arr_size(); ++i) {
                out.add(arr.get(i));
            }
            out.add(new_el);
        }
        return new_val(std::move(out).to_datum());
    }
};

class append_term_t : public pend_term_t {
public:
    append_term_t(compile_env_t *env, const raw_term_t &term)
        : pend_term_t(env, term) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env,
                                       args_t *args,
                                       UNUSED eval_flags_t flags) const {
        return pend(env, args, AP);
    }
    virtual const char *name() const { return "append"; }
};

class prepend_term_t : public pend_term_t {
public:
    prepend_term_t(compile_env_t *env, const raw_term_t &term)
        : pend_term_t(env, term) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        return pend(env, args, PRE);
    }
    virtual const char *name() const { return "prepend"; }
};

// This gets the literal index of a (possibly negative) index relative to a
// fixed size.
static uint64_t canonicalize(const term_t *t, int64_t index, size_t size, bool *oob_out = 0) {
    CT_ASSERT(sizeof(size_t) <= sizeof(uint64_t));
    if (index >= 0) return index;
    if (uint64_t(index * -1) > size) {
        if (oob_out) {
            *oob_out = true;
        } else {
            rfail_target(t, base_exc_t::NON_EXISTENCE,
                         "Index out of bounds: %" PRIi64, index);
        }
        return 0;
    }
    return uint64_t(size) + index;
}

// needed because nth_term_impl may need to recurse over its contents to deal with
// e.g. grouped data.
scoped_ptr_t<val_t> nth_term_direct_impl(const term_t *term,
                                         scope_env_t *env,
                                         scoped_ptr_t<val_t> aggregate,
                                         const val_t *index) {
    int32_t n = index->as_int<int32_t>();
    if (aggregate->get_type().is_convertible(val_t::type_t::DATUM)) {
        datum_t arr = aggregate->as_datum();
        size_t real_n = canonicalize(term, n, arr.arr_size());
        return term->new_val(arr.get(real_n));
    } else {
        counted_t<table_t> tbl;
        counted_t<datum_stream_t> s;
        if (aggregate->get_type().is_convertible(val_t::type_t::SELECTION)) {
            auto selection = aggregate->as_selection(env->env);
            tbl = selection->table;
            s = selection->seq;
        } else {
            s = aggregate->as_seq(env->env);
        }
        rcheck_target(term,
                      n >= -1,
                      base_exc_t::LOGIC,
                      strprintf("Cannot use an index < -1 (%d) on a stream.", n));

        batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env->env);
        if (n != -1) {
            batchspec = batchspec.with_at_most(int64_t(n)+1);
        }

        datum_t last_d;
        {
            profile::sampler_t sampler("Find nth element.", env->env->trace);
            for (int32_t i = 0; ; ++i) {
                sampler.new_sample();
                datum_t d = s->next(env->env, batchspec);
                if (!d.has()) {
                    rcheck_target(
                        term, n == -1 && last_d.has(), base_exc_t::NON_EXISTENCE,
                        strprintf("Index out of bounds: %d", n));
                    return tbl.has()
                        ? term->new_val(single_selection_t::from_row(
                                            env->env, term->backtrace(), tbl, last_d))
                        : term->new_val(last_d);
                }
                if (i == n) {
                    return tbl.has()
                        ? term->new_val(single_selection_t::from_row(
                                            env->env, term->backtrace(), tbl, d))
                        : term->new_val(d);
                }
                last_d = d;
                r_sanity_check(n == -1 || i < n);
            }
        }
    }
}

scoped_ptr_t<val_t> nth_term_impl(const term_t *term, scope_env_t *env,
                                  scoped_ptr_t<val_t> aggregate,
                                  const scoped_ptr_t<val_t> &index) {
    if (aggregate->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
        counted_t<datum_stream_t> seq = aggregate->as_seq(env->env);
        if (seq->is_grouped()) {
            counted_t<grouped_data_t> result
                = seq->to_array(env->env)->as_grouped_data();
            // (aggregate is empty, because maybe_grouped_data sets at most one of
            // gd and aggregate, so we don't have to worry about re-evaluating it.
            counted_t<grouped_data_t> out(new grouped_data_t());
            for (auto kv = result->begin(); kv != result->end(); ++kv) {
                scoped_ptr_t<val_t> value
                    = make_scoped<val_t>(kv->second, aggregate->backtrace());
                (*out)[kv->first] = nth_term_direct_impl(
                        term, env, std::move(value), index.get())->as_datum();
            }
            return make_scoped<val_t>(out, term->backtrace());
        } else {
            return nth_term_direct_impl(term, env, std::move(aggregate), index.get());
        }
    } else {
        return nth_term_direct_impl(term, env, std::move(aggregate), index.get());
    }
}

class nth_term_t : public op_term_t {
public:
    nth_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    friend class bracket_t;
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        return nth_term_impl(this, env, args->arg(env, 0), args->arg(env, 1));
    }
    virtual const char *name() const { return "nth"; }
    virtual bool is_grouped_seq_op() const { return true; }
};

class is_empty_term_t : public op_term_t {
public:
    is_empty_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        batchspec_t batchspec =
            batchspec_t::user(batch_type_t::NORMAL, env->env).with_at_most(1);
        bool is_empty = !args->arg(env, 0)->as_seq(env->env)->next(env->env, batchspec).has();
        return new_val(datum_t::boolean(is_empty));
    }
    virtual const char *name() const { return "is_empty"; }
};

// TODO: this kinda sucks.
class slice_term_t : public bounded_op_term_t {
public:
    slice_term_t(compile_env_t *env, const raw_term_t &term)
        : bounded_op_term_t(env, term, argspec_t(2, 3)) { }
private:

    void canon_helper(size_t size, bool index_open, int64_t fake_index,
                      bool is_left, uint64_t *real_index_out) const {
        bool index_oob = false;
        *real_index_out = canonicalize(this, fake_index, size, &index_oob);
        if (index_open == is_left && !index_oob) {
            *real_index_out += 1; // This is safe because it was an int64_t before.
        }
    }

    scoped_ptr_t<val_t> slice_array(datum_t arr,
                                    const configured_limits_t &limits,
                                    bool left_open, int64_t fake_l,
                                    bool right_open, int64_t fake_r) const {
        uint64_t real_l, real_r;
        canon_helper(arr.arr_size(), left_open, fake_l, true, &real_l);
        canon_helper(arr.arr_size(), right_open, fake_r, false, &real_r);

        datum_array_builder_t out(limits);
        for (uint64_t i = real_l; i < real_r; ++i) {
            if (i >= arr.arr_size()) {
                break;
            }
            out.add(arr.get(i));
        }
        return new_val(std::move(out).to_datum());
    }

    scoped_ptr_t<val_t> slice_binary(datum_t binary,
                                     bool left_open, int64_t fake_l,
                                     bool right_open, int64_t fake_r) const {
        const datum_string_t &data = binary.as_binary();
        uint64_t real_l, real_r;
        canon_helper(data.size(), left_open, fake_l, true, &real_l);
        canon_helper(data.size(), right_open, fake_r, false, &real_r);

        real_r = clamp<uint64_t>(real_r, 0, data.size());

        datum_string_t subdata;
        if (real_l <= real_r) {
            subdata = datum_string_t(real_r - real_l, &data.data()[real_l]);
        } else {
            subdata = datum_string_t();
        }

        return new_val(datum_t::binary(std::move(subdata)));
    }

    scoped_ptr_t<val_t> slice_string(datum_t str,
                                     bool left_open, int64_t fake_l,
                                     bool right_open, int64_t fake_r) const {
        const datum_string_t &data = str.as_str();
        size_t size = utf8::count_codepoints(data);
        uint64_t real_l, real_r;
        canon_helper(size, left_open, fake_l, true, &real_l);
        canon_helper(size, right_open, fake_r, false, &real_r);

        real_r = clamp<uint64_t>(real_r, 0, size);

        datum_string_t subdata;
        if (real_l <= real_r) {
            size_t from = utf8::index_codepoints(data, real_l);
            size_t to = utf8::index_codepoints(data, real_r);
            subdata = datum_string_t(to - from, &data.data()[from]);
        } else {
            subdata = datum_string_t();
        }

        return new_val(datum_t(std::move(subdata)));
    }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v = args->arg(env, 0);
        bool left_open = is_left_open(env, args);
        int64_t fake_l = args->arg(env, 1)->as_int<int64_t>();
        bool right_open = args->num_args() == 3 ? is_right_open(env, args) : false;
        int64_t fake_r = args->num_args() == 3 ? args->arg(env, 2)->as_int<int64_t>() : -1;

        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            datum_t d = v->as_datum();
            if (d.get_type() == datum_t::R_ARRAY) {
                return slice_array(d, env->env->limits(), left_open, fake_l,
                                   right_open, fake_r);
            } else if (d.get_type() == datum_t::R_BINARY) {
                return slice_binary(d, left_open, fake_l, right_open, fake_r);
            } else if (d.get_type() == datum_t::R_STR) {
                return slice_string(d, left_open, fake_l, right_open, fake_r);
            } else {
                rfail_target(v, base_exc_t::LOGIC,
                             "Expected ARRAY, BINARY, or STRING, but found %s.",
                             d.get_type_name().c_str());
            }
        } else if (v->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
            counted_t<table_t> t;
            counted_t<datum_stream_t> seq;
            if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
                auto selection = v->as_selection(env->env);
                t = selection->table;
                seq = selection->seq;
            } else {
                seq = v->as_seq(env->env);
            }

            rcheck(fake_l >= 0, base_exc_t::LOGIC,
                   "Cannot use a negative left index on a stream.");
            uint64_t real_l = fake_l;
            if (left_open) {
                real_l += 1; // This is safe because it was an int64_t before.
            }
            uint64_t real_r = fake_r;
            if (fake_r < -1) {
                rfail(base_exc_t::LOGIC,
                      "Cannot use a right index < -1 on a stream.");
            } else if (fake_r == -1) {
                rcheck(!right_open, base_exc_t::LOGIC,
                       "Cannot slice to an open right index of -1 on a stream.");
                real_r = std::numeric_limits<uint64_t>::max();
            } else if (!right_open) {
                real_r += 1;  // This is safe because it was an int32_t before.
            }
            counted_t<datum_stream_t> new_ds = seq->slice(real_l, real_r);
            return t.has()
                ? new_val(make_counted<selection_t>(t, new_ds))
                : new_val(env->env, new_ds);
        } else {
            rcheck_typed_target(v, false, "Cannot slice non-sequences.");
        }
        unreachable();
    }
    virtual const char *name() const { return "slice"; }
};

class limit_term_t : public op_term_t {
public:
    limit_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v = args->arg(env, 0);
        counted_t<table_t> t;
        counted_t<datum_stream_t> ds;
        if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
            auto selection = v->as_selection(env->env);
            t = selection->table;
            ds = selection->seq;
        } else {
            ds = v->as_seq(env->env);
        }
        int32_t r = args->arg(env, 1)->as_int<int32_t>();
        rcheck(r >= 0, base_exc_t::LOGIC,
               strprintf("LIMIT takes a non-negative argument (got %d)", r));
        counted_t<datum_stream_t> new_ds = ds->slice(0, r);
        return t.has()
            ? new_val(make_counted<selection_t>(t, new_ds))
            : new_val(env->env, new_ds);
    }
    virtual const char *name() const { return "limit"; }
};

class set_insert_term_t : public op_term_t {
public:
    set_insert_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {
        datum_t arr = args->arg(env, 0)->as_datum();
        datum_t new_el = args->arg(env, 1)->as_datum();
        // We only use el_set for equality purposes, so the reql_version doesn't
        // really matter (with respect to datum ordering behavior).  But we play it
        // safe.
        std::set<datum_t, optional_datum_less_t> el_set;
        datum_array_builder_t out(env->env->limits());
        for (size_t i = 0; i < arr.arr_size(); ++i) {
            if (el_set.insert(arr.get(i)).second) {
                out.add(arr.get(i));
            }
        }
        if (!std_contains(el_set, new_el)) {
            out.add(new_el);
        }

        return new_val(std::move(out).to_datum());
    }

    virtual const char *name() const { return "set_insert"; }
};

class set_union_term_t : public op_term_t {
public:
    set_union_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {
        datum_t arr1 = args->arg(env, 0)->as_datum();
        datum_t arr2 = args->arg(env, 1)->as_datum();
        // The reql_version doesn't actually matter here -- we only use the datum
        // comparisons for equality purposes.
        std::set<datum_t, optional_datum_less_t> el_set;
        datum_array_builder_t out(env->env->limits());
        for (size_t i = 0; i < arr1.arr_size(); ++i) {
            if (el_set.insert(arr1.get(i)).second) {
                out.add(arr1.get(i));
            }
        }
        for (size_t i = 0; i < arr2.arr_size(); ++i) {
            if (el_set.insert(arr2.get(i)).second) {
                out.add(arr2.get(i));
            }
        }

        return new_val(std::move(out).to_datum());
    }

    virtual const char *name() const { return "set_union"; }
};

class set_intersection_term_t : public op_term_t {
public:
    set_intersection_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {
        datum_t arr1 = args->arg(env, 0)->as_datum();
        datum_t arr2 = args->arg(env, 1)->as_datum();
        // The reql_version here doesn't really matter.  We only use el_set
        // comparison for equality purposes.
        std::set<datum_t, optional_datum_less_t> el_set;
        datum_array_builder_t out(env->env->limits());
        for (size_t i = 0; i < arr1.arr_size(); ++i) {
            el_set.insert(arr1.get(i));
        }
        for (size_t i = 0; i < arr2.arr_size(); ++i) {
            if (std_contains(el_set, arr2.get(i))) {
                out.add(arr2.get(i));
                el_set.erase(arr2.get(i));
            }
        }

        return new_val(std::move(out).to_datum());
    }

    virtual const char *name() const { return "set_intersection"; }
};

class set_difference_term_t : public op_term_t {
public:
    set_difference_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(
        scope_env_t *env, args_t *args, eval_flags_t) const {
        datum_t arr1 = args->arg(env, 0)->as_datum();
        datum_t arr2 = args->arg(env, 1)->as_datum();
        // The reql_version here doesn't really matter.  We only use el_set
        // comparison for equality purposes.
        std::set<datum_t, optional_datum_less_t> el_set;
        datum_array_builder_t out(env->env->limits());
        for (size_t i = 0; i < arr2.arr_size(); ++i) {
            el_set.insert(arr2.get(i));
        }
        for (size_t i = 0; i < arr1.arr_size(); ++i) {
            if (!std_contains(el_set, arr1.get(i))) {
                out.add(arr1.get(i));
                el_set.insert(arr1.get(i));
            }
        }

        return new_val(std::move(out).to_datum());
    }

    virtual const char *name() const { return "set_difference"; }
};

class at_term_t : public op_term_t {
public:
    /* This is a bit of a pain here. Some array operations are referencing
     * elements of the array (such as change_at and delete_at) while others are
     * actually referencing the spaces between the elements (such as insert_at
     * and splice_at). This distinction changes how we canonicalize negative
     * indexes so we need to make it here. */
    enum index_method_t { ELEMENTS, SPACES};

    at_term_t(compile_env_t *env, const raw_term_t &term,
              argspec_t argspec, index_method_t index_method)
        : op_term_t(env, term, argspec), index_method_(index_method) { }

    virtual void modify(scope_env_t *env, args_t *args, size_t index,
                        datum_array_builder_t *array) const = 0;

    scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        datum_array_builder_t arr(args->arg(env, 0)->as_datum(), env->env->limits());
        size_t index;
        if (index_method_ == ELEMENTS) {
            index = canonicalize(this, args->arg(env, 1)->as_datum().as_int(), arr.size());
        } else if (index_method_ == SPACES) {
            index = canonicalize(this, args->arg(env, 1)->as_datum().as_int(), arr.size() + 1);
        } else {
            unreachable();
        }

        modify(env, args, index, &arr);
        return new_val(std::move(arr).to_datum());
    }
private:
    index_method_t index_method_;
};

class insert_at_term_t : public at_term_t {
public:
    insert_at_term_t(compile_env_t *env, const raw_term_t &term)
        : at_term_t(env, term, argspec_t(3), SPACES) { }
private:
    void modify(scope_env_t *env, args_t *args, size_t index,
                datum_array_builder_t *array) const {
        datum_t new_el = args->arg(env, 2)->as_datum();
        array->insert(index, new_el);
    }
    const char *name() const { return "insert_at"; }
};


class splice_at_term_t : public at_term_t {
public:
    splice_at_term_t(compile_env_t *env, const raw_term_t &term)
        : at_term_t(env, term, argspec_t(3), SPACES) { }
private:
    void modify(scope_env_t *env, args_t *args, size_t index,
                datum_array_builder_t *array) const {
        datum_t new_els = args->arg(env, 2)->as_datum();
        array->splice(index, new_els);
    }
    const char *name() const { return "splice_at"; }
};

class delete_at_term_t : public at_term_t {
public:
    delete_at_term_t(compile_env_t *env, const raw_term_t &term)
        : at_term_t(env, term, argspec_t(2, 3), ELEMENTS) { }
private:
    void modify(scope_env_t *env, args_t *args, size_t index,
                datum_array_builder_t *array) const {
        if (args->num_args() == 2) {
            array->erase(index);
        } else {
            int end_index = canonicalize(
                this, args->arg(env, 2)->as_datum().as_int(), array->size());
            array->erase_range(index, end_index);
        }
    }
    const char *name() const { return "delete_at"; }
};

class change_at_term_t : public at_term_t {
public:
    change_at_term_t(compile_env_t *env, const raw_term_t &term)
        : at_term_t(env, term, argspec_t(3), ELEMENTS) { }
private:
    void modify(scope_env_t *env, args_t *args, size_t index,
                datum_array_builder_t *array) const {
        datum_t new_el = args->arg(env, 2)->as_datum();
        array->change(index, new_el);
    }
    const char *name() const { return "change_at"; }
};

class offsets_of_term_t : public op_term_t {
public:
    offsets_of_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v = args->arg(env, 1);
        counted_t<const func_t> fun;
        if (v->get_type().is_convertible(val_t::type_t::FUNC)) {
            fun = v->as_func();
        } else {
            fun = new_eq_comparison_func(v->as_datum(), backtrace());
        }
        return new_val(env->env, args->arg(env, 0)->as_seq(env->env)->offsets_of(fun));
    }
    virtual const char *name() const { return "offsets_of"; }
};

class contains_term_t : public op_term_t {
public:
    contains_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<datum_stream_t> seq = args->arg(env, 0)->as_seq(env->env);
        std::vector<datum_t> required_els;
        std::vector<counted_t<const func_t> > required_funcs;
        for (size_t i = 1; i < args->num_args(); ++i) {
            scoped_ptr_t<val_t> v = args->arg(env, i);
            if (v->get_type().is_convertible(val_t::type_t::FUNC)) {
                required_funcs.push_back(v->as_func());
            } else {
                required_els.push_back(v->as_datum());
            }
        }
        // This needs to be a terminal batch to avoid pathological behavior in
        // the worst case.
        batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env->env);
        {
            profile::sampler_t sampler("Evaluating elements in contains.",
                                       env->env->trace);
            datum_t el;
            while (el = seq->next(env->env, batchspec), el.has()) {
                for (auto it = required_els.begin(); it != required_els.end(); ++it) {
                    if (*it == el) {
                        std::swap(*it, required_els.back());
                        required_els.pop_back();
                        break; // Bag semantics for contains.
                    }
                }
                for (auto it = required_funcs.begin();
                     it != required_funcs.end();
                     ++it) {
                    if ((*it)->call(env->env, el)->as_bool()) {
                        std::swap(*it, required_funcs.back());
                        required_funcs.pop_back();
                        break; // Bag semantics for contains.
                    }
                }
                if (required_els.size() == 0 && required_funcs.size() == 0) {
                    return new_val_bool(true);
                }
                sampler.new_sample();
            }
        }
        return new_val_bool(false);
    }
    virtual const char *name() const { return "contains"; }
};

class args_term_t : public op_term_t {
public:
    args_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(1)) { }
    // This just evaluates its argument and returns it as an array.  The actual
    // logic to make `args` splice arguments is in op.cc.
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env,
                                          args_t *args,
                                          eval_flags_t eval_flags) const {
        scoped_ptr_t<val_t> v0 = args->arg(env, 0, eval_flags);
        // If v0 is not an array, force a type error.
        v0->as_datum().check_type(datum_t::R_ARRAY);
        return v0;
    }
private:
    virtual const char *name() const { return "args"; }
};

counted_t<term_t> make_args_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<args_term_t>(env, term);
}

counted_t<term_t> make_contains_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<contains_term_t>(env, term);
}

counted_t<term_t> make_append_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<append_term_t>(env, term);
}

counted_t<term_t> make_prepend_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<prepend_term_t>(env, term);
}

counted_t<term_t> make_nth_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<nth_term_t>(env, term);
}

counted_t<term_t> make_is_empty_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<is_empty_term_t>(env, term);
}

counted_t<term_t> make_slice_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<slice_term_t>(env, term);
}

counted_t<term_t> make_limit_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<limit_term_t>(env, term);
}

counted_t<term_t> make_set_insert_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<set_insert_term_t>(env, term);
}

counted_t<term_t> make_set_union_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<set_union_term_t>(env, term);
}

counted_t<term_t> make_set_intersection_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<set_intersection_term_t>(env, term);
}

counted_t<term_t> make_set_difference_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<set_difference_term_t>(env, term);
}

counted_t<term_t> make_insert_at_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<insert_at_term_t>(env, term);
}

counted_t<term_t> make_delete_at_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<delete_at_term_t>(env, term);
}

counted_t<term_t> make_change_at_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<change_at_term_t>(env, term);
}

counted_t<term_t> make_splice_at_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<splice_at_term_t>(env, term);
}

counted_t<term_t> make_offsets_of_term(
    compile_env_t *env, const raw_term_t &term) {
    return make_counted<offsets_of_term_t>(env, term);
}


}  // namespace ql
