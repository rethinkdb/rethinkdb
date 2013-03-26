#ifndef RDB_PROTOCOL_TERMS_WRITES_HPP_
#define RDB_PROTOCOL_TERMS_WRITES_HPP_

#include <string>
#include <utility>
#include <vector>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

// This function is used by e.g. foreach to merge statistics from multiple write
// operations.
const datum_t *stats_merge(env_t *env, UNUSED const std::string &key,
                           const datum_t *l, const datum_t *r,
                           const rcheckable_t *caller) {
    if (l->get_type() == datum_t::R_NUM && r->get_type() == datum_t::R_NUM) {
        //debugf("%s %s %s -> %s\n", key.c_str(), l->print().c_str(), r->print().c_str(),
        //       env->add_ptr(new datum_t(l->as_num() + r->as_num()))->print().c_str());
        return env->add_ptr(new datum_t(l->as_num() + r->as_num()));
    } else if (l->get_type() == datum_t::R_ARRAY && r->get_type() == datum_t::R_ARRAY) {
        datum_t *arr = env->add_ptr(new datum_t(datum_t::R_ARRAY));
        for (size_t i = 0; i < l->size(); ++i) arr->add(l->get(i));
        for (size_t i = 0; i < r->size(); ++i) arr->add(r->get(i));
        return arr;
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
const datum_t *pure_merge(UNUSED env_t *env, UNUSED const std::string &key,
                          UNUSED const datum_t *l, UNUSED const datum_t *r,
                          UNUSED const rcheckable_t *caller) {
    r_sanity_check(false);
    return 0;
}

static const char *const insert_optargs[] = {"upsert"};
class insert_term_t : public op_term_t {
public:
    insert_term_t(env_t *env, const Term *term)
        : op_term_t(env, term, argspec_t(2), optargspec_t(insert_optargs)) { }
private:

    void maybe_generate_key(table_t *tbl,
                            std::vector<std::string> *generated_keys_out,
                            const datum_t **datum_out) {
        if (!(*datum_out)->get(tbl->get_pkey(), NOTHROW)) {
            std::string key = uuid_to_str(generate_uuid());
            const datum_t *keyd = env->add_ptr(new datum_t(key));
            datum_t *d = env->add_ptr(new datum_t(datum_t::R_OBJECT));
            bool conflict = d->add(tbl->get_pkey(), keyd);
            r_sanity_check(!conflict);
            *datum_out = (*datum_out)->merge(env, d, pure_merge);
            generated_keys_out->push_back(key);
        }
    }

    virtual val_t *eval_impl() {
        table_t *t = arg(0)->as_table();
        val_t *upsert_val = optarg("upsert", 0);
        bool upsert = upsert_val ? upsert_val->as_bool() : false;

        bool done = false;
        const datum_t *stats = env->add_ptr(new datum_t(datum_t::R_OBJECT));
        std::vector<std::string> generated_keys;
        val_t *v1 = arg(1);
        if (v1->get_type().is_convertible(val_t::type_t::DATUM)) {
            const datum_t *d = v1->as_datum();
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
            datum_stream_t *ds = v1->as_seq();
            while (const datum_t *d = ds->next()) {
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
            datum_t *genkeys = env->add_ptr(new datum_t(datum_t::R_ARRAY));
            for (size_t i = 0; i < generated_keys.size(); ++i) {
                genkeys->add(env->add_ptr(new datum_t(generated_keys[i])));
            }
            datum_t *d = env->add_ptr(new datum_t(datum_t::R_OBJECT));
            UNUSED bool b = d->add("generated_keys", genkeys);
            stats = stats->merge(env, d, pure_merge);
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
    virtual val_t *eval_impl() {
        bool nondet_ok = false;
        if (val_t *v = optarg("non_atomic", 0)) nondet_ok = v->as_bool();
        func_t *f = arg(1)->as_func(IDENTITY_SHORTCUT);
        rcheck((f->is_deterministic() || nondet_ok),
               "Could not prove function deterministic.  "
               "Maybe you want to use the non_atomic flag?");

        val_t *v0 = arg(0);
        if (v0->get_type().is_convertible(val_t::type_t::SINGLE_SELECTION)) {
            std::pair<table_t *, const datum_t *> tblrow = v0->as_single_selection();
            return new_val(tblrow.first->replace(tblrow.second, f, nondet_ok));
        }
        std::pair<table_t *, datum_stream_t *> tblrows = v0->as_selection();
        table_t *tbl = tblrows.first;
        datum_stream_t *ds = tblrows.second;
        const datum_t *stats = env->add_ptr(new datum_t(datum_t::R_OBJECT));
        while (const datum_t *d = ds->next()) {
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
    virtual val_t *eval_impl() {
        const char *fail_msg = "FOREACH expects one or more write queries.";

        datum_stream_t *ds = arg(0)->as_seq();
        const datum_t *stats = env->add_ptr(new datum_t(datum_t::R_OBJECT));
        while (const datum_t *row = ds->next()) {
            val_t *v = arg(1)->as_func(IDENTITY_SHORTCUT)->call(row);
            try {
                const datum_t *d = v->as_datum();
                if (d->get_type() == datum_t::R_OBJECT) {
                    stats = stats->merge(env, d, stats_merge);
                } else {
                    for (size_t i = 0; i < d->size(); ++i) {
                        stats = stats->merge(env, d->get(i), stats_merge);
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
