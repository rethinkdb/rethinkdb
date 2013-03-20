#ifndef RDB_PROTOCOL_TERMS_WRITES_HPP_
#define RDB_PROTOCOL_TERMS_WRITES_HPP_

#include <string>
#include <utility>
#include <vector>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {

// RSI env is unused in stats_merge and pure_merge.

// This function is used by e.g. foreach to merge statistics from multiple write
// operations.
counted_t<const datum_t> stats_merge(UNUSED env_t *env, UNUSED const std::string &key,
                                     counted_t<const datum_t> l, counted_t<const datum_t> r,
                                     const rcheckable_t *caller) {
    if (l->get_type() == datum_t::R_NUM && r->get_type() == datum_t::R_NUM) {
        //debugf("%s %s %s -> %s\n", key.c_str(), l->print().c_str(), r->print().c_str(),
        //       env->add_ptr(new datum_t(l->as_num() + r->as_num()))->print().c_str());
        return make_counted<datum_t>(l->as_num() + r->as_num());
    } else if (l->get_type() == datum_t::R_ARRAY && r->get_type() == datum_t::R_ARRAY) {
        scoped_ptr_t<datum_t> arr(new datum_t(datum_t::R_ARRAY));
        for (size_t i = 0; i < l->size(); ++i) {
            arr->add(l->el(i));
        }
        for (size_t i = 0; i < r->size(); ++i) {
            arr->add(r->el(i));
        }
        return counted_t<const datum_t>(arr.release());
    }

    // Merging a string is left-preferential, which is just a no-op.
    rcheck_target(
        caller, l->get_type() == datum_t::R_STR && r->get_type() == datum_t::R_STR,
        strprintf("Cannot merge statistics of type %s/%s -- what are you doing?",
                  l->get_type_name(), r->get_type_name()));
    // debugf("%s %s %s -> %s\n", key.c_str(), l->print().c_str(), r->print().c_str(),
    //        l->print().c_str());
    return l;
}

// Use this merge if it should theoretically never be called.
counted_t<const datum_t> pure_merge(UNUSED env_t *env, UNUSED const std::string &key,
                                    UNUSED counted_t<const datum_t> l, UNUSED counted_t<const datum_t> r,
                                    UNUSED const rcheckable_t *caller) {
    r_sanity_check(false);
    return counted_t<const datum_t>();
}

static const char *const insert_optargs[] = {"upsert"};
class insert_term_t : public op_term_t {
public:
    insert_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(2), optargspec_t(insert_optargs)) { }
private:

    void maybe_generate_key(counted_t<table_t> tbl,
                            std::vector<std::string> *generated_keys_out,
                            counted_t<const datum_t> *datum_out) {
        if (!(*datum_out)->el(tbl->get_pkey(), NOTHROW)) {
            std::string key = uuid_to_str(generate_uuid());
            counted_t<const datum_t> keyd(new datum_t(key));
            scoped_ptr_t<datum_t> d(new datum_t(datum_t::R_OBJECT));
            bool conflict = d->add(tbl->get_pkey(), keyd);
            r_sanity_check(!conflict);
            *datum_out = (*datum_out)->merge(env, counted_t<const datum_t>(d.release()), pure_merge);
            generated_keys_out->push_back(key);
        }
    }

    virtual counted_t<val_t> eval_impl() {
        counted_t<table_t> t = arg(0)->as_table();
        counted_t<val_t> upsert_val = optarg("upsert", counted_t<val_t>());
        bool upsert = upsert_val ? upsert_val->as_bool() : false;

        bool done = false;
        counted_t<const datum_t> stats(new datum_t(datum_t::R_OBJECT));
        std::vector<std::string> generated_keys;
        counted_t<val_t> v1 = arg(1);
        if (v1->get_type().is_convertible(val_t::type_t::DATUM)) {
            counted_t<const datum_t> d = v1->as_datum();
            if (d->get_type() == datum_t::R_OBJECT) {
                try {
                    maybe_generate_key(t, &generated_keys, &d);
                } catch (const any_ql_exc_t &) {
                    // We just ignore it, the same error will be handled in `replace`.
                    // TODO: that solution sucks.
                }
                stats = stats->merge(env, t->replace(d, d, upsert), stats_merge);
                done = true;
            }
        }

        if (!done) {
            counted_t<datum_stream_t> ds = v1->as_seq();
            while (counted_t<const datum_t> d = ds->next()) {
                try {
                    maybe_generate_key(t, &generated_keys, &d);
                } catch (const any_ql_exc_t &) {
                    // We just ignore it, the same error will be handled in `replace`.
                    // TODO: that solution sucks.
                }
                stats = stats->merge(env, t->replace(d, d, upsert), stats_merge);
            }
        }

        if (generated_keys.size() > 0) {
            scoped_ptr_t<datum_t> genkeys(new datum_t(datum_t::R_ARRAY));
            for (size_t i = 0; i < generated_keys.size(); ++i) {
                genkeys->add(make_counted<datum_t>(generated_keys[i]));
            }
            scoped_ptr_t<datum_t> d(new datum_t(datum_t::R_OBJECT));
            UNUSED bool b = d->add("generated_keys", counted_t<const datum_t>(genkeys.release()));
            stats = stats->merge(env, counted_t<const datum_t>(d.release()), pure_merge);
        }

        return new_val(stats);
    }
    virtual const char *name() const { return "insert"; }
};

static const char *const replace_optargs[] = {"non_atomic"};
class replace_term_t : public op_term_t {
public:
    replace_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(2), optargspec_t(replace_optargs)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        bool nondet_ok = false;
        if (counted_t<val_t> v = optarg("non_atomic", counted_t<val_t>())) {
            nondet_ok = v->as_bool();
        }
        counted_t<func_t> f = arg(1)->as_func(IDENTITY_SHORTCUT);
        rcheck((f->is_deterministic() || nondet_ok),
               "Could not prove function deterministic.  "
               "Maybe you want to use the non_atomic flag?");

        counted_t<val_t> v0 = arg(0);
        if (v0->get_type().is_convertible(val_t::type_t::SINGLE_SELECTION)) {
            std::pair<counted_t<table_t>, counted_t<const datum_t> > tblrow = v0->as_single_selection();
            return new_val(tblrow.first->replace(tblrow.second, f, nondet_ok));
        }
        std::pair<counted_t<table_t> , counted_t<datum_stream_t> > tblrows = v0->as_selection();
        counted_t<table_t> tbl = tblrows.first;
        counted_t<datum_stream_t> ds = tblrows.second;
        counted_t<const datum_t> stats(new datum_t(datum_t::R_OBJECT));
        while (counted_t<const datum_t> d = ds->next()) {
            stats = stats->merge(env, tbl->replace(d, f, nondet_ok), stats_merge);
        }
        return new_val(stats);
    }
    virtual const char *name() const { return "replace"; }
};

// DELETE and UPDATE are in rewrites.hpp

class foreach_term_t : public op_term_t {
public:
    foreach_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl() {
        const char *fail_msg = "FOREACH expects one or more write queries.";

        counted_t<datum_stream_t> ds = arg(0)->as_seq();
        counted_t<const datum_t> stats(new datum_t(datum_t::R_OBJECT));
        while (counted_t<const datum_t> row = ds->next()) {
            counted_t<val_t> v = arg(1)->as_func(IDENTITY_SHORTCUT)->call(row);
            try {
                counted_t<const datum_t> d = v->as_datum();
                if (d->get_type() == datum_t::R_OBJECT) {
                    stats = stats->merge(env, d, stats_merge);
                } else {
                    for (size_t i = 0; i < d->size(); ++i) {
                        stats = stats->merge(env, d->el(i), stats_merge);
                    }
                }
            } catch (const exc_t &e) {
                throw exc_t(fail_msg, e.backtrace);
            } catch (const datum_exc_t &de) {
                rfail_target(v, "%s", fail_msg);
            }
        }
        return new_val(stats);
    }
    virtual const char *name() const { return "foreach"; }
};

} // namespace ql

#endif // RDB_PROTOCOL_TERMS_WRITES_HPP_
