#include "rdb_protocol/terms/terms.hpp"

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"

namespace ql {

class pend_term_t : public op_term_t {
public:
    pend_term_t(env_t *env, protob_t<const Term> term) : op_term_t(env, term, argspec_t(2)) { }
protected:
    enum which_pend_t {PRE, AP};
    counted_t<val_t> pend(which_pend_t which_pend) {
        counted_t<const datum_t> arr = arg(0)->as_datum();
        counted_t<const datum_t> new_el = arg(1)->as_datum();
        scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_ARRAY));
        if (which_pend == PRE) {
            // TODO: this is horrendously inefficient.
            out->add(new_el);
            for (size_t i = 0; i < arr->size(); ++i) out->add(arr->get(i));
        } else {
            // TODO: this is horrendously inefficient.
            for (size_t i = 0; i < arr->size(); ++i) out->add(arr->get(i));
            out->add(new_el);
        }
        return new_val(counted_t<const datum_t>(out.release()));
    }
};

class append_term_t : public pend_term_t {
public:
    append_term_t(env_t *env, protob_t<const Term> term) : pend_term_t(env, term) { }
private:
    virtual counted_t<val_t> eval_impl() {
        return pend(AP);
    }
    virtual const char *name() const { return "append"; }
};

class prepend_term_t : public pend_term_t {
public:
    prepend_term_t(env_t *env, protob_t<const Term> term) : pend_term_t(env, term) { }
private:
    virtual counted_t<val_t> eval_impl() {
        return pend(PRE);
    }
    virtual const char *name() const { return "prepend"; }
};

// This gets the literal index of a (possibly negative) index relative to a
// fixed size.
size_t canonicalize(const term_t *t, int32_t index, size_t size, bool *oob_out = 0) {
    CT_ASSERT(sizeof(size_t) >= sizeof(int32_t));
    if (index >= 0) return index;
    if (size_t(index * -1) > size) {
        if (oob_out) {
            *oob_out = true;
        } else {
            rfail_target(t, base_exc_t::NON_EXISTENCE, "Index out of bounds: %d", index);
        }
        return 0;
    }
    return size + index;
}

class nth_term_t : public op_term_t {
public:
    nth_term_t(env_t *env, protob_t<const Term> term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> v = arg(0);
        int32_t n = arg(1)->as_int<int32_t>();
        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            counted_t<const datum_t> arr = v->as_datum();
            size_t real_n = canonicalize(this, n, arr->size());
            return new_val(arr->get(real_n));
        } else {
            counted_t<datum_stream_t> s = v->as_seq();
            rcheck(n >= -1,
                   base_exc_t::GENERIC,
                   strprintf("Cannot use an index < -1 (%d) on a stream.", n));

            counted_t<const datum_t> last_d;
            for (int32_t i = 0; ; ++i) {
                counted_t<const datum_t> d = s->next();
                if (!d.has()) {
                    rcheck(n == -1 && last_d.has(), base_exc_t::GENERIC,
                           strprintf("Index out of bounds: %d", n));
                    return new_val(last_d);
                }
                if (i == n) return new_val(d);
                last_d = d;
                r_sanity_check(n == -1 || i < n);
            }
        }
    }
    virtual const char *name() const { return "nth"; }
};

class is_empty_term_t : public op_term_t {
public:
    is_empty_term_t(env_t *env, protob_t<const Term> term) :
        op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl() {
      bool emptyp = ! arg(0)->as_seq()->next().has();
      return new_val(make_counted<const datum_t>(datum_t::type_t::R_BOOL, emptyp));
    }
    virtual const char *name() const { return "is_empty"; }
};

// TODO: this kinda sucks.
class slice_term_t : public op_term_t {
public:
    slice_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(3)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> v = arg(0);
        int32_t fake_l = arg(1)->as_int<int32_t>();
        int32_t fake_r = arg(2)->as_int<int32_t>();
        if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
            counted_t<const datum_t> arr = v->as_datum();
            arr->check_type(datum_t::R_ARRAY);
            bool l_oob = false;
            size_t real_l = canonicalize(this, fake_l, arr->size(), &l_oob);
            if (l_oob) real_l = 0;
            bool r_oob = false;
            size_t real_r = canonicalize(this, fake_r, arr->size(), &r_oob);

            scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_ARRAY));
            if (!r_oob) {
                for (size_t i = real_l; i <= real_r; ++i) {
                    if (i >= arr->size()) break;
                    out->add(arr->get(i));
                }
            }
            return new_val(counted_t<const datum_t>(out.release()));
        } else if (v->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
            counted_t<table_t> t;
            counted_t<datum_stream_t> seq;
            if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
                std::pair<counted_t<table_t>, counted_t<datum_stream_t> > t_seq
                    = v->as_selection();
                t = t_seq.first;
                seq = t_seq.second;
            } else {
                seq = v->as_seq();
            }

            rcheck(fake_l >= 0, base_exc_t::GENERIC,
                   "Cannot use a negative left index on a stream.");
            rcheck(fake_r >= -1, base_exc_t::GENERIC,
                   "Cannot use a right index < -1 on a stream");
            counted_t<datum_stream_t> new_ds = seq->slice(fake_l, fake_r);
            return t.has() ? new_val(new_ds, t) : new_val(new_ds);
        }
        rcheck_typed_target(v, false, "Cannot slice non-sequences.");
        unreachable();
    }
    virtual const char *name() const { return "slice"; }
};

class limit_term_t : public op_term_t {
public:
    limit_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> v = arg(0);
        counted_t<table_t> t;
        if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
            t = v->as_selection().first;
        }
        counted_t<datum_stream_t> ds = v->as_seq();
        int32_t r = arg(1)->as_int<int32_t>();
        rcheck(r >= 0, base_exc_t::GENERIC,
               strprintf("LIMIT takes a non-negative argument (got %d)", r));
        counted_t<datum_stream_t> new_ds;
        if (r == 0) {
            new_ds = ds->slice(1, 0); // (0, -1) has a different meaning
        } else {
            new_ds = ds->slice(0, r-1); // note that both bounds are inclusive
        }
        return t.has() ? new_val(new_ds, t) : new_val(new_ds);
    }
    virtual const char *name() const { return "limit"; }
};

class set_insert_term_t : public op_term_t {
public:
    set_insert_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<const datum_t> arr = arg(0)->as_datum();
        counted_t<const datum_t> new_el = arg(1)->as_datum();
        std::set<counted_t<const datum_t> > el_set;
        scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_ARRAY));
        for (size_t i = 0; i < arr->size(); ++i) {
            if (el_set.insert(arr->get(i)).second) {
                out->add(arr->get(i));
            }
        }
        if (!std_contains(el_set, new_el)) {
            out->add(new_el);
        }

        return new_val(counted_t<const datum_t>(out.release()));
    }

    virtual const char *name() const { return "set_insert"; }
};

class set_union_term_t : public op_term_t {
public:
    set_union_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<const datum_t> arr1 = arg(0)->as_datum();
        counted_t<const datum_t> arr2 = arg(1)->as_datum();
        std::set<counted_t<const datum_t> > el_set;
        scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_ARRAY));
        for (size_t i = 0; i < arr1->size(); ++i) {
            if (el_set.insert(arr1->get(i)).second) {
                out->add(arr1->get(i));
            }
        }
        for (size_t i = 0; i < arr2->size(); ++i) {
            if (el_set.insert(arr2->get(i)).second) {
                out->add(arr2->get(i));
            }
        }

        return new_val(counted_t<const datum_t>(out.release()));
    }

    virtual const char *name() const { return "set_union"; }
};

class set_intersection_term_t : public op_term_t {
public:
    set_intersection_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<const datum_t> arr1 = arg(0)->as_datum();
        counted_t<const datum_t> arr2 = arg(1)->as_datum();
        std::set<counted_t<const datum_t> > el_set;
        scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_ARRAY));
        for (size_t i = 0; i < arr1->size(); ++i) {
            el_set.insert(arr1->get(i));
        }
        for (size_t i = 0; i < arr2->size(); ++i) {
            if (std_contains(el_set, arr2->get(i))) {
                out->add(arr2->get(i));
                el_set.erase(arr2->get(i));
            }
        }

        return new_val(counted_t<const datum_t>(out.release()));
    }

    virtual const char *name() const { return "set_intersection"; }
};

class set_difference_term_t : public op_term_t {
public:
    set_difference_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<const datum_t> arr1 = arg(0)->as_datum();
        counted_t<const datum_t> arr2 = arg(1)->as_datum();
        std::set<counted_t<const datum_t> > el_set;
        scoped_ptr_t<datum_t> out(new datum_t(datum_t::R_ARRAY));
        for (size_t i = 0; i < arr2->size(); ++i) {
            el_set.insert(arr2->get(i));
        }
        for (size_t i = 0; i < arr1->size(); ++i) {
            if (!std_contains(el_set, arr1->get(i))) {
                out->add(arr1->get(i));
                el_set.insert(arr1->get(i));
            }
        }

        return new_val(counted_t<const datum_t>(out.release()));
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

    at_term_t(env_t *env, protob_t<const Term> term,
              argspec_t argspec, index_method_t index_method)
        : op_term_t(env, term, argspec), index_method_(index_method) { }
    virtual void modify(size_t index, datum_t *array) = 0;
    counted_t<val_t> eval_impl() {
        scoped_ptr_t<datum_t> arr(new datum_t(arg(0)->as_datum()->as_array()));
        size_t index;
        if (index_method_ == ELEMENTS) {
            index = canonicalize(this, arg(1)->as_datum()->as_int(), arr->size());
        } else if (index_method_ == SPACES) {
            index = canonicalize(this, arg(1)->as_datum()->as_int(), arr->size() + 1);
        } else {
            unreachable();
        }

        modify(index, arr.get());
        return new_val(counted_t<const datum_t>(arr.release()));
    }
private:
    index_method_t index_method_;
};

class insert_at_term_t : public at_term_t {
public:
    insert_at_term_t(env_t *env, protob_t<const Term> term)
        : at_term_t(env, term, argspec_t(3), SPACES) { }
private:
    void modify(size_t index, datum_t *array) {
        counted_t<const datum_t> new_el = arg(2)->as_datum();
        array->insert(index, new_el);
    }
    const char *name() const { return "insert_at"; }
};


class splice_at_term_t : public at_term_t {
public:
    splice_at_term_t(env_t *env, protob_t<const Term> term)
        : at_term_t(env, term, argspec_t(3), SPACES) { }
private:
    void modify(size_t index, datum_t *array) {
        counted_t<const datum_t> new_els = arg(2)->as_datum();
        array->splice(index, new_els);
    }
    const char *name() const { return "splice_at"; }
};

class delete_at_term_t : public at_term_t {
public:
    delete_at_term_t(env_t *env, protob_t<const Term> term)
        : at_term_t(env, term, argspec_t(2, 3), ELEMENTS) { }
private:
    void modify(size_t index, datum_t *array) {
        if (num_args() == 2) {
            array->erase(index);
        } else {
            int end_index = canonicalize(this, arg(2)->as_datum()->as_int(), array->size());
            array->erase_range(index, end_index);
        }
    }
    const char *name() const { return "delete_at"; }
};

class change_at_term_t : public at_term_t {
public:
    change_at_term_t(env_t *env, protob_t<const Term> term)
        : at_term_t(env, term, argspec_t(3), ELEMENTS) { }
private:
    void modify(size_t index, datum_t *array) {
        counted_t<const datum_t> new_el = arg(2)->as_datum();
        array->change(index, new_el);
    }
    const char *name() const { return "change_at"; }
};

class indexes_of_term_t : public op_term_t {
public:
    indexes_of_term_t(env_t *env, protob_t<const Term> term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> v = arg(1);
        counted_t<func_t> fun;
        if (v->get_type().is_convertible(val_t::type_t::FUNC)) {
            fun = v->as_func();
        } else {
            fun = func_t::new_eq_comparison_func(env, v->as_datum(), backtrace());
        }
        return new_val(arg(0)->as_seq()->indexes_of(fun));
    }
    virtual const char *name() const { return "indexes_of"; }
};

class contains_term_t : public op_term_t {
public:
    contains_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        counted_t<datum_stream_t> seq = arg(0)->as_seq();
        std::vector<counted_t<const datum_t> > required_els;
        for (size_t i = 1; i < num_args(); ++i) {
            required_els.push_back(arg(i)->as_datum());
        }
        while (counted_t<const datum_t> el = seq->next()) {
            for (auto it = required_els.begin(); it != required_els.end(); ++it) {
                if (**it == *el) {
                    std::swap(*it, required_els.back());
                    required_els.pop_back();
                    break; // Bag semantics for contains.
                }
            }
            if (required_els.size() == 0) {
                return new_val_bool(true);
            }
        }
        return new_val_bool(false);
    }
    virtual const char *name() const { return "contains"; }
};


counted_t<term_t> make_contains_term(env_t *env, protob_t<const Term> term) {
    return make_counted<contains_term_t>(env, term);
}

counted_t<term_t> make_append_term(env_t *env, protob_t<const Term> term) {
    return make_counted<append_term_t>(env, term);
}

counted_t<term_t> make_prepend_term(env_t *env, protob_t<const Term> term) {
    return make_counted<prepend_term_t>(env, term);
}

counted_t<term_t> make_nth_term(env_t *env, protob_t<const Term> term) {
    return make_counted<nth_term_t>(env, term);
}

counted_t<term_t> make_is_empty_term(env_t *env, protob_t<const Term> term) {
    return make_counted<is_empty_term_t>(env, term);
}

counted_t<term_t> make_slice_term(env_t *env, protob_t<const Term> term) {
    return make_counted<slice_term_t>(env, term);
}

counted_t<term_t> make_limit_term(env_t *env, protob_t<const Term> term) {
    return make_counted<limit_term_t>(env, term);
}

counted_t<term_t> make_set_insert_term(env_t *env, protob_t<const Term> term) {
    return make_counted<set_insert_term_t>(env, term);
}

counted_t<term_t> make_set_union_term(env_t *env, protob_t<const Term> term) {
    return make_counted<set_union_term_t>(env, term);
}

counted_t<term_t> make_set_intersection_term(env_t *env, protob_t<const Term> term) {
    return make_counted<set_intersection_term_t>(env, term);
}

counted_t<term_t> make_set_difference_term(env_t *env, protob_t<const Term> term) {
    return make_counted<set_difference_term_t>(env, term);
}

counted_t<term_t> make_insert_at_term(env_t *env, protob_t<const Term> term) {
    return make_counted<insert_at_term_t>(env, term);
}

counted_t<term_t> make_delete_at_term(env_t *env, protob_t<const Term> term) {
    return make_counted<delete_at_term_t>(env, term);
}

counted_t<term_t> make_change_at_term(env_t *env, protob_t<const Term> term) {
    return make_counted<change_at_term_t>(env, term);
}

counted_t<term_t> make_splice_at_term(env_t *env, protob_t<const Term> term) {
    return make_counted<splice_at_term_t>(env, term);
}

counted_t<term_t> make_indexes_of_term(env_t *env, protob_t<const Term> term) {
    return make_counted<indexes_of_term_t>(env, term);
}


}  // namespace ql
