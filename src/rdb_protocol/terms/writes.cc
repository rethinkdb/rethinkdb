
#include <string>
#include <utility>
#include <vector>

#include "rdb_protocol/func.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"

namespace ql {

// This function is used by e.g. foreach to merge statistics from multiple write
// operations.
counted_t<const datum_t> stats_merge(UNUSED const std::string &key,
                                     counted_t<const datum_t> l,
                                     counted_t<const datum_t> r,
                                     const rcheckable_t *caller) {
    if (l->get_type() == datum_t::R_NUM && r->get_type() == datum_t::R_NUM) {
        return make_counted<datum_t>(l->as_num() + r->as_num());
    } else if (l->get_type() == datum_t::R_ARRAY && r->get_type() == datum_t::R_ARRAY) {
        datum_ptr_t arr(datum_t::R_ARRAY);
        for (size_t i = 0; i < l->size(); ++i) {
            arr.add(l->get(i));
        }
        for (size_t i = 0; i < r->size(); ++i) {
            arr.add(r->get(i));
        }
        return arr.to_counted();
    }

    // Merging a string is left-preferential, which is just a no-op.
    rcheck_target(
        caller, base_exc_t::GENERIC,
        l->get_type() == datum_t::R_STR && r->get_type() == datum_t::R_STR,
        strprintf("Cannot merge statistics `%s` (type %s) and `%s` (type %s).",
                  l->trunc_print().c_str(), l->get_type_name().c_str(),
                  r->trunc_print().c_str(), r->get_type_name().c_str()));
    return l;
}

// Use this merge if it should theoretically never be called.
counted_t<const datum_t> pure_merge(UNUSED const std::string &key,
                                    UNUSED counted_t<const datum_t> l,
                                    UNUSED counted_t<const datum_t> r,
                                    UNUSED const rcheckable_t *caller) {
    r_sanity_check(false);
    return counted_t<const datum_t>();
}

counted_t<const datum_t> new_stats_object() {
    datum_ptr_t stats(datum_t::R_OBJECT);
    const char *const keys[] =
        {"inserted", "deleted", "skipped", "replaced", "unchanged", "errors"};
    for (size_t i = 0; i < sizeof(keys)/sizeof(*keys); ++i) {
        UNUSED bool b = stats.add(keys[i], make_counted<datum_t>(0.0));
    }
    return stats.to_counted();
}

durability_requirement_t parse_durability_optarg(counted_t<val_t> arg,
                                                 pb_rcheckable_t *target) {
    if (!arg.has()) { return DURABILITY_REQUIREMENT_DEFAULT; }
    std::string str = arg->as_str();
    if (str == "hard") { return DURABILITY_REQUIREMENT_HARD; }
    if (str == "soft") { return DURABILITY_REQUIREMENT_SOFT; }
    rfail_target(target,
                 base_exc_t::GENERIC,
                 "Durability option `%s` unrecognized "
                 "(options are \"hard\" and \"soft\").",
                 str.c_str());
    unreachable();
}


class insert_term_t : public op_term_t {
public:
    insert_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2),
                    optargspec_t({"upsert", "durability", "return_vals"})) { }

private:
    void maybe_generate_key(counted_t<table_t> tbl,
                            std::vector<std::string> *generated_keys_out,
                            counted_t<const datum_t> *datum_out) {
        if (!(*datum_out)->get(tbl->get_pkey(), NOTHROW).has()) {
            std::string key = uuid_to_str(generate_uuid());
            counted_t<const datum_t> keyd(new datum_t(std::string(key)));
            datum_ptr_t d(datum_t::R_OBJECT);
            bool conflict = d.add(tbl->get_pkey(), keyd);
            r_sanity_check(!conflict);
            *datum_out = (*datum_out)->merge(d.to_counted(), pure_merge);
            generated_keys_out->push_back(key);
        }
    }

    virtual counted_t<val_t> eval_impl(env_t *env, UNUSED eval_flags_t flags) {
        counted_t<table_t> t = arg(env, 0)->as_table();
        counted_t<val_t> upsert_val = optarg(env, "upsert");
        bool upsert = upsert_val.has() ? upsert_val->as_bool() : false;
        counted_t<val_t> return_vals_val = optarg(env, "return_vals");
        bool return_vals = return_vals_val.has() ? return_vals_val->as_bool() : false;

        const durability_requirement_t durability_requirement
            = parse_durability_optarg(optarg(env, "durability"), this);

        bool done = false;
        counted_t<const datum_t> stats = new_stats_object();
        std::vector<std::string> generated_keys;
        counted_t<val_t> v1 = arg(env, 1);
        if (v1->get_type().is_convertible(val_t::type_t::DATUM)) {
            counted_t<const datum_t> d = v1->as_datum();
            if (d->get_type() == datum_t::R_OBJECT) {
                try {
                    maybe_generate_key(t, &generated_keys, &d);
                } catch (const base_exc_t &) {
                    // We just ignore it, the same error will be handled in `replace`.
                    // TODO: that solution sucks.
                }
                counted_t<const datum_t> new_stats =
                    t->replace(env, d, d, upsert, durability_requirement, return_vals);
                stats = stats->merge(new_stats, stats_merge);
                done = true;
            }
        }

        if (!done) {
            counted_t<datum_stream_t> datum_stream = v1->as_seq(env);
            rcheck(!return_vals, base_exc_t::GENERIC,
                   "Optarg RETURN_VALS is invalid for multi-row inserts.");

            for (;;) {
                std::vector<counted_t<const datum_t> > datums
                    = datum_stream->next_batch(env);
                if (datums.empty()) {
                    break;
                }

                for (auto it = datums.begin(); it != datums.end(); ++it) {
                    try {
                        maybe_generate_key(t, &generated_keys, &*it);
                    } catch (const base_exc_t &) {
                        // We just ignore it, the same error will be handled in
                        // `replace`.  TODO: that solution sucks.
                    }
                }

                std::vector<counted_t<const datum_t> > results =
                    t->batch_replace(env, datums, datums, upsert, durability_requirement);
                for (auto it = results.begin(); it != results.end(); ++it) {
                    stats = stats->merge(*it, stats_merge);
                }
            }
        }

        if (generated_keys.size() > 0) {
            std::vector<counted_t<const datum_t> > genkeys;
            genkeys.reserve(generated_keys.size());
            for (size_t i = 0; i < generated_keys.size(); ++i) {
                genkeys.push_back(make_counted<datum_t>(std::move(generated_keys[i])));
            }
            datum_ptr_t d(datum_t::R_OBJECT);
            UNUSED bool b = d.add("generated_keys", make_counted<datum_t>(std::move(genkeys)));
            stats = stats->merge(d.to_counted(), pure_merge);
        }

        return new_val(stats);
    }
    virtual const char *name() const { return "insert"; }
};

class replace_term_t : public op_term_t {
public:
    replace_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2),
                    optargspec_t({"non_atomic", "durability", "return_vals"})) { }

private:
    virtual counted_t<val_t> eval_impl(env_t *env, UNUSED eval_flags_t flags) {
        bool nondet_ok = false;
        if (counted_t<val_t> v = optarg(env, "non_atomic")) {
            nondet_ok = v->as_bool();
        }
        bool return_vals = false;
        if (counted_t<val_t> v = optarg(env, "return_vals")) {
            return_vals = v->as_bool();
        }

        const durability_requirement_t durability_requirement
            = parse_durability_optarg(optarg(env, "durability"), this);

        counted_t<func_t> f = arg(env, 1)->as_func(env, CONSTANT_SHORTCUT);
        if (!nondet_ok) {
            f->assert_deterministic("Maybe you want to use the non_atomic flag?");
        }

        counted_t<val_t> v0 = arg(env, 0);
        counted_t<const datum_t> stats = new_stats_object();
        if (v0->get_type().is_convertible(val_t::type_t::SINGLE_SELECTION)) {
            std::pair<counted_t<table_t>, counted_t<const datum_t> > tblrow
                = v0->as_single_selection();
            counted_t<const datum_t> result =
                tblrow.first->replace(env, tblrow.second, f, nondet_ok,
                                      durability_requirement, return_vals);
            stats = stats->merge(result, stats_merge);
        } else {
            std::pair<counted_t<table_t>, counted_t<datum_stream_t> > tblrows
                = v0->as_selection(env);
            counted_t<table_t> tbl = tblrows.first;
            counted_t<datum_stream_t> ds = tblrows.second;

            rcheck(!return_vals, base_exc_t::GENERIC,
                   "Optarg RETURN_VALS is invalid for multi-row modifications.");

            for (;;) {
                std::vector<counted_t<const datum_t> > datums = ds->next_batch(env);
                if (datums.empty()) {
                    break;
                }
                std::vector<counted_t<const datum_t> > results =
                    tbl->batch_replace(env, datums, f, nondet_ok, durability_requirement);

                for (auto result = results.begin(); result != results.end(); ++result) {
                    stats = stats->merge(*result, stats_merge);
                }
            }
        }
        return new_val(stats);
    }

    virtual const char *name() const { return "replace"; }
};

// DELETE and UPDATE are in rewrites.hpp

class foreach_term_t : public op_term_t {
public:
    foreach_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }

private:
    virtual counted_t<val_t> eval_impl(env_t *env, UNUSED eval_flags_t flags) {
        const char *fail_msg = "FOREACH expects one or more basic write queries.";

        counted_t<datum_stream_t> ds = arg(env, 0)->as_seq(env);
        counted_t<const datum_t> stats(new datum_t(datum_t::R_OBJECT));
        while (counted_t<const datum_t> row = ds->next(env)) {
            counted_t<val_t> v = arg(env, 1)->as_func(env, CONSTANT_SHORTCUT)->call(row);
            try {
                counted_t<const datum_t> d = v->as_datum();
                if (d->get_type() == datum_t::R_OBJECT) {
                    stats = stats->merge(d, stats_merge);
                } else {
                    for (size_t i = 0; i < d->size(); ++i) {
                        stats = stats->merge(d->get(i), stats_merge);
                    }
                }
            } catch (const exc_t &e) {
                throw exc_t(e.get_type(), fail_msg, e.backtrace());
            } catch (const datum_exc_t &de) {
                rfail_target(v, base_exc_t::GENERIC, "%s  %s", fail_msg, de.what());
            }
        }
        return new_val(stats);
    }

    virtual const char *name() const { return "foreach"; }
};

counted_t<term_t> make_insert_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<insert_term_t>(env, term);
}
counted_t<term_t> make_replace_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<replace_term_t>(env, term);
}
counted_t<term_t> make_foreach_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<foreach_term_t>(env, term);
}

} // namespace ql
