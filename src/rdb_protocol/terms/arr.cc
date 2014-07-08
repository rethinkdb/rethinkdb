// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "stl_utils.hpp"

#include "debug.hpp"

namespace ql {

class pend_term_t : public op_term_t {
public:
    pend_term_t(compile_env_t *env, const protob_t<const Term> &term) : op_term_t(env, term, argspec_t(2)) { }
protected:
    enum which_pend_t { PRE, AP };

    counted_t<val_t> pend(scope_env_t *env, args_t *args, which_pend_t which_pend) const {
        counted_t<const datum_t> arr = args->arg(env, 0)->as_datum();
        counted_t<const datum_t> new_el = args->arg(env, 1)->as_datum();
        datum_ptr_t out(datum_t::R_ARRAY);
        if (which_pend == PRE) {
            // TODO: this is horrendously inefficient.
            out.add(new_el);
            for (size_t i = 0; i < arr->size(); ++i) out.add(arr->get(i));
        } else {
            // TODO: this is horrendously inefficient.
            for (size_t i = 0; i < arr->size(); ++i) out.add(arr->get(i));
            out.add(new_el);
        }
        return new_val(out.to_counted());
    }
};

class append_term_t : public pend_term_t {
public:
    append_term_t(compile_env_t *env, const protob_t<const Term> &term) : pend_term_t(env, term) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env,
                                       args_t *args,
                                       UNUSED eval_flags_t flags) const {
        return pend(env, args, AP);
    }
    virtual const char *name() const { return "append"; }
};

class prepend_term_t : public pend_term_t {
public:
    prepend_term_t(compile_env_t *env, const protob_t<const Term> &term) : pend_term_t(env, term) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        return pend(env, args, PRE);
    }
    virtual const char *name() const { return "prepend"; }
};

// This gets the literal index of a (possibly negative) index relative to a
// fixed size.
uint64_t canonicalize(const term_t *t, int64_t index, size_t size, bool *oob_out = 0) {
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

class nth_term_t : public op_term_t {
public:
    nth_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> v = args->arg(env, 0);
        int32_t n = args->arg(env, 1)->as_int<int32_t>();
        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            counted_t<const datum_t> arr = v->as_datum();
            size_t real_n = canonicalize(this, n, arr->size());
            return new_val(arr->get(real_n));
        } else {
            counted_t<table_t> tbl;
            counted_t<datum_stream_t> s;
            if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
                auto pair = v->as_selection(env->env);
                tbl = pair.first;
                s = pair.second;
            } else {
                s = v->as_seq(env->env);
            }
            rcheck(n >= -1,
                   base_exc_t::GENERIC,
                   strprintf("Cannot use an index < -1 (%d) on a stream.", n));

            batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env->env);
            if (n != -1) {
                batchspec = batchspec.with_at_most(int64_t(n)+1);
            }

            counted_t<const datum_t> last_d;
            {
                profile::sampler_t sampler("Find nth element.", env->env->trace);
                for (int32_t i = 0; ; ++i) {
                    sampler.new_sample();
                    counted_t<const datum_t> d = s->next(env->env, batchspec);
                    if (!d.has()) {
                        rcheck(n == -1 && last_d.has(), base_exc_t::NON_EXISTENCE,
                               strprintf("Index out of bounds: %d", n));
                        return tbl.has() ? new_val(last_d, tbl) : new_val(last_d);
                    }
                    if (i == n) {
                        return tbl.has() ? new_val(d, tbl) : new_val(d);
                    }
                    last_d = d;
                    r_sanity_check(n == -1 || i < n);
                }
            }
        }
    }
    virtual const char *name() const { return "nth"; }
};

class is_empty_term_t : public op_term_t {
public:
    is_empty_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        batchspec_t batchspec = batchspec_t::user(batch_type_t::NORMAL, env->env);
        bool is_empty = !args->arg(env, 0)->as_seq(env->env)->next(env->env, batchspec).has();
        return new_val(make_counted<const datum_t>(datum_t::type_t::R_BOOL, is_empty));
    }
    virtual const char *name() const { return "is_empty"; }
};

// TODO: this kinda sucks.
class slice_term_t : public bounded_op_term_t {
public:
    slice_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : bounded_op_term_t(env, term, argspec_t(2, 3)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> v = args->arg(env, 0);
        bool left_open = is_left_open(env, args);
        int64_t fake_l = args->arg(env, 1)->as_int<int64_t>();
        bool right_open = args->num_args() == 3 ? is_right_open(env, args) : false;
        int64_t fake_r = args->num_args() == 3 ? args->arg(env, 2)->as_int<int64_t>() : -1;

        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            counted_t<const datum_t> arr = v->as_datum();
            arr->check_type(datum_t::R_ARRAY);
            bool l_oob = false;
            uint64_t real_l = canonicalize(this, fake_l, arr->size(), &l_oob);
            if (l_oob) {
                real_l = 0;
            } else if (left_open) {
                real_l += 1; // This is safe because it was an int64_t before.
            }
            bool r_oob = false;
            uint64_t real_r = canonicalize(this, fake_r, arr->size(), &r_oob);
            if (r_oob) {
                return new_val(make_counted<const datum_t>(datum_t::R_ARRAY));
            } else if (!right_open) {
                real_r += 1; // This is safe because it was an int64_t before.
            }

            datum_ptr_t out(datum_t::R_ARRAY);
            if (!r_oob) {
                for (uint64_t i = real_l; i < real_r; ++i) {
                    if (i >= arr->size()) break;
                    out.add(arr->get(i));
                }
            }
            return new_val(out.to_counted());
        } else if (v->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
            counted_t<table_t> t;
            counted_t<datum_stream_t> seq;
            if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
                std::pair<counted_t<table_t>, counted_t<datum_stream_t> > t_seq
                    = v->as_selection(env->env);
                t = t_seq.first;
                seq = t_seq.second;
            } else {
                seq = v->as_seq(env->env);
            }

            rcheck(fake_l >= 0, base_exc_t::GENERIC,
                   "Cannot use a negative left index on a stream.");
            uint64_t real_l = fake_l;
            if (left_open) {
                real_l += 1; // This is safe because it was an int64_t before.
            }
            uint64_t real_r = fake_r;
            if (fake_r < -1) {
                rfail(base_exc_t::GENERIC,
                      "Cannot use a right index < -1 on a stream.");
            } else if (fake_r == -1) {
                rcheck(!right_open, base_exc_t::GENERIC,
                       "Cannot slice to an open right index of -1 on a stream.");
                real_r = std::numeric_limits<uint64_t>::max();
            } else if (!right_open) {
                real_r += 1;  // This is safe because it was an int32_t before.
            }
            counted_t<datum_stream_t> new_ds = seq->slice(real_l, real_r);
            return t.has() ? new_val(new_ds, t) : new_val(env->env, new_ds);
        } else {
            rcheck_typed_target(v, false, "Cannot slice non-sequences.");
        }
        unreachable();
    }
    virtual const char *name() const { return "slice"; }
};

class limit_term_t : public op_term_t {
public:
    limit_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> v = args->arg(env, 0);
        counted_t<table_t> t;
        if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
            t = v->as_selection(env->env).first;
        }
        counted_t<datum_stream_t> ds = v->as_seq(env->env);
        int32_t r = args->arg(env, 1)->as_int<int32_t>();
        rcheck(r >= 0, base_exc_t::GENERIC,
               strprintf("LIMIT takes a non-negative argument (got %d)", r));
        counted_t<datum_stream_t> new_ds = ds->slice(0, r);
        return t.has() ? new_val(new_ds, t) : new_val(env->env, new_ds);
    }
    virtual const char *name() const { return "limit"; }
};

class set_insert_term_t : public op_term_t {
public:
    set_insert_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<const datum_t> arr = args->arg(env, 0)->as_datum();
        counted_t<const datum_t> new_el = args->arg(env, 1)->as_datum();
        std::set<counted_t<const datum_t> > el_set;
        datum_ptr_t out(datum_t::R_ARRAY);
        for (size_t i = 0; i < arr->size(); ++i) {
            if (el_set.insert(arr->get(i)).second) {
                out.add(arr->get(i));
            }
        }
        if (!std_contains(el_set, new_el)) {
            out.add(new_el);
        }

        return new_val(out.to_counted());
    }

    virtual const char *name() const { return "set_insert"; }
};

class set_union_term_t : public op_term_t {
public:
    set_union_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<const datum_t> arr1 = args->arg(env, 0)->as_datum();
        counted_t<const datum_t> arr2 = args->arg(env, 1)->as_datum();
        std::set<counted_t<const datum_t> > el_set;
        datum_ptr_t out(datum_t::R_ARRAY);
        for (size_t i = 0; i < arr1->size(); ++i) {
            if (el_set.insert(arr1->get(i)).second) {
                out.add(arr1->get(i));
            }
        }
        for (size_t i = 0; i < arr2->size(); ++i) {
            if (el_set.insert(arr2->get(i)).second) {
                out.add(arr2->get(i));
            }
        }

        return new_val(out.to_counted());
    }

    virtual const char *name() const { return "set_union"; }
};

class set_intersection_term_t : public op_term_t {
public:
    set_intersection_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<const datum_t> arr1 = args->arg(env, 0)->as_datum();
        counted_t<const datum_t> arr2 = args->arg(env, 1)->as_datum();
        std::set<counted_t<const datum_t> > el_set;
        datum_ptr_t out(datum_t::R_ARRAY);
        for (size_t i = 0; i < arr1->size(); ++i) {
            el_set.insert(arr1->get(i));
        }
        for (size_t i = 0; i < arr2->size(); ++i) {
            if (std_contains(el_set, arr2->get(i))) {
                out.add(arr2->get(i));
                el_set.erase(arr2->get(i));
            }
        }

        return new_val(out.to_counted());
    }

    virtual const char *name() const { return "set_intersection"; }
};

class set_difference_term_t : public op_term_t {
public:
    set_difference_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<const datum_t> arr1 = args->arg(env, 0)->as_datum();
        counted_t<const datum_t> arr2 = args->arg(env, 1)->as_datum();
        std::set<counted_t<const datum_t> > el_set;
        datum_ptr_t out(datum_t::R_ARRAY);
        for (size_t i = 0; i < arr2->size(); ++i) {
            el_set.insert(arr2->get(i));
        }
        for (size_t i = 0; i < arr1->size(); ++i) {
            if (!std_contains(el_set, arr1->get(i))) {
                out.add(arr1->get(i));
                el_set.insert(arr1->get(i));
            }
        }

        return new_val(out.to_counted());
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

    at_term_t(compile_env_t *env, protob_t<const Term> term,
              argspec_t argspec, index_method_t index_method)
        : op_term_t(env, term, argspec), index_method_(index_method) { }

    virtual void modify(scope_env_t *env, args_t *args, size_t index, datum_ptr_t *array) const = 0;

    counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        datum_ptr_t arr(args->arg(env, 0)->as_datum()->as_array());
        size_t index;
        if (index_method_ == ELEMENTS) {
            index = canonicalize(this, args->arg(env, 1)->as_datum()->as_int(), arr->size());
        } else if (index_method_ == SPACES) {
            index = canonicalize(this, args->arg(env, 1)->as_datum()->as_int(), arr->size() + 1);
        } else {
            unreachable();
        }

        modify(env, args, index, &arr);
        return new_val(arr.to_counted());
    }
private:
    index_method_t index_method_;
};

class insert_at_term_t : public at_term_t {
public:
    insert_at_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : at_term_t(env, term, argspec_t(3), SPACES) { }
private:
    void modify(scope_env_t *env, args_t *args, size_t index, datum_ptr_t *array) const {
        counted_t<const datum_t> new_el = args->arg(env, 2)->as_datum();
        array->insert(index, new_el);
    }
    const char *name() const { return "insert_at"; }
};


class splice_at_term_t : public at_term_t {
public:
    splice_at_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : at_term_t(env, term, argspec_t(3), SPACES) { }
private:
    void modify(scope_env_t *env, args_t *args, size_t index, datum_ptr_t *array) const {
        counted_t<const datum_t> new_els = args->arg(env, 2)->as_datum();
        array->splice(index, new_els);
    }
    const char *name() const { return "splice_at"; }
};

class delete_at_term_t : public at_term_t {
public:
    delete_at_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : at_term_t(env, term, argspec_t(2, 3), ELEMENTS) { }
private:
    void modify(scope_env_t *env, args_t *args, size_t index, datum_ptr_t *array) const {
        if (args->num_args() == 2) {
            array->erase(index);
        } else {
            int end_index =
                canonicalize(this, args->arg(env, 2)->as_datum()->as_int(), (*array)->size());
            array->erase_range(index, end_index);
        }
    }
    const char *name() const { return "delete_at"; }
};

class change_at_term_t : public at_term_t {
public:
    change_at_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : at_term_t(env, term, argspec_t(3), ELEMENTS) { }
private:
    void modify(scope_env_t *env, args_t *args, size_t index, datum_ptr_t *array) const {
        counted_t<const datum_t> new_el = args->arg(env, 2)->as_datum();
        array->change(index, new_el);
    }
    const char *name() const { return "change_at"; }
};

class indexes_of_term_t : public op_term_t {
public:
    indexes_of_term_t(compile_env_t *env, const protob_t<const Term> &term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> v = args->arg(env, 1);
        counted_t<func_t> fun;
        if (v->get_type().is_convertible(val_t::type_t::FUNC)) {
            fun = v->as_func();
        } else {
            fun = new_eq_comparison_func(v->as_datum(), backtrace());
        }
        return new_val(env->env, args->arg(env, 0)->as_seq(env->env)->indexes_of(fun));
    }
    virtual const char *name() const { return "indexes_of"; }
};

class contains_term_t : public op_term_t {
public:
    contains_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<datum_stream_t> seq = args->arg(env, 0)->as_seq(env->env);
        std::vector<counted_t<const datum_t> > required_els;
        std::vector<counted_t<func_t> > required_funcs;
        for (size_t i = 1; i < args->num_args(); ++i) {
            counted_t<val_t> v = args->arg(env, i);
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
            while (counted_t<const datum_t> el = seq->next(env->env, batchspec)) {
                for (auto it = required_els.begin(); it != required_els.end(); ++it) {
                    if (**it == *el) {
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
    args_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
    // This just evaluates its argument and returns it as an array.  The actual
    // logic to make `args` splice arguments is in op.cc.
    virtual counted_t<val_t> eval_impl(scope_env_t *env,
                                       args_t *args,
                                       eval_flags_t eval_flags) const {
        counted_t<val_t> v0 = args->arg(env, 0, eval_flags);
        v0->as_datum()->as_array(); // If v0 is not an array, force a type error.
        return v0;
    }
private:
    virtual const char *name() const { return "args"; }
};

counted_t<term_t> make_args_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<args_term_t>(env, term);
}

counted_t<term_t> make_contains_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<contains_term_t>(env, term);
}

counted_t<term_t> make_append_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<append_term_t>(env, term);
}

counted_t<term_t> make_prepend_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<prepend_term_t>(env, term);
}

counted_t<term_t> make_nth_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<nth_term_t>(env, term);
}

counted_t<term_t> make_is_empty_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<is_empty_term_t>(env, term);
}

counted_t<term_t> make_slice_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<slice_term_t>(env, term);
}

counted_t<term_t> make_limit_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<limit_term_t>(env, term);
}

counted_t<term_t> make_set_insert_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<set_insert_term_t>(env, term);
}

counted_t<term_t> make_set_union_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<set_union_term_t>(env, term);
}

counted_t<term_t> make_set_intersection_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<set_intersection_term_t>(env, term);
}

counted_t<term_t> make_set_difference_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<set_difference_term_t>(env, term);
}

counted_t<term_t> make_insert_at_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<insert_at_term_t>(env, term);
}

counted_t<term_t> make_delete_at_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<delete_at_term_t>(env, term);
}

counted_t<term_t> make_change_at_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<change_at_term_t>(env, term);
}

counted_t<term_t> make_splice_at_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<splice_at_term_t>(env, term);
}

counted_t<term_t> make_indexes_of_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<indexes_of_term_t>(env, term);
}


}  // namespace ql
