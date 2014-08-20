// Copyright 2010-2013 RethinkDB, all rights reserved.

#include <string>
#include <utility>
#include <vector>

#include "rdb_protocol/func.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"

namespace ql {

// Use this merge if it should theoretically never be called.
counted_t<const datum_t> pure_merge(UNUSED const std::string &key,
                                    UNUSED counted_t<const datum_t> l,
                                    UNUSED counted_t<const datum_t> r,
                                    UNUSED const configured_limits_t &limits,
                                    UNUSED std::set<std::string> *conditions) {
    r_sanity_check(false);
    return counted_t<const datum_t>();
}

counted_t<const datum_t> new_stats_object() {
    datum_object_builder_t stats;
    const char *const keys[] =
        {"inserted", "deleted", "skipped", "replaced", "unchanged", "errors"};
    for (size_t i = 0; i < sizeof(keys)/sizeof(*keys); ++i) {
        UNUSED bool b = stats.add(keys[i], make_counted<datum_t>(0.0));
    }
    return std::move(stats).to_counted();
}

conflict_behavior_t parse_conflict_optarg(counted_t<val_t> arg,
                                          const pb_rcheckable_t *target) {
    if (!arg.has()) { return conflict_behavior_t::ERROR; }
    const wire_string_t &str = arg->as_str();
    if (str == "error") { return conflict_behavior_t::ERROR; }
    if (str == "replace") { return conflict_behavior_t::REPLACE; }
    if (str == "update") { return conflict_behavior_t::UPDATE; }
    rfail_target(target,
                 base_exc_t::GENERIC,
                 "Conflict option `%s` unrecognized "
                 "(options are \"error\", \"replace\" and \"update\").",
                 str.c_str());
}

durability_requirement_t parse_durability_optarg(counted_t<val_t> arg,
                                                 const pb_rcheckable_t *target) {
    if (!arg.has()) { return DURABILITY_REQUIREMENT_DEFAULT; }
    const wire_string_t &str = arg->as_str();
    if (str == "hard") { return DURABILITY_REQUIREMENT_HARD; }
    if (str == "soft") { return DURABILITY_REQUIREMENT_SOFT; }
    rfail_target(target,
                 base_exc_t::GENERIC,
                 "Durability option `%s` unrecognized "
                 "(options are \"hard\" and \"soft\").",
                 str.c_str());
}

class insert_term_t : public op_term_t {
public:
    insert_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2),
                    optargspec_t({"conflict", "durability", "return_vals", "return_changes"})) { }

private:
    static void maybe_generate_key(counted_t<table_t> tbl,
                                   const configured_limits_t &limits,
                                   std::vector<std::string> *generated_keys_out,
                                   size_t *keys_skipped_out,
                                   counted_t<const datum_t> *datum_out) {
        if (!(*datum_out)->get(tbl->get_pkey(), NOTHROW).has()) {
            std::string key = uuid_to_str(generate_uuid());
            counted_t<const datum_t> keyd(new datum_t(std::string(key)));
            {
                datum_object_builder_t d;
                bool conflict = d.add(tbl->get_pkey(), keyd);
                r_sanity_check(!conflict);
                std::set<std::string> conditions;
                *datum_out = (*datum_out)->merge(std::move(d).to_counted(), pure_merge,
                                                limits, &conditions);
                // we happen to know that pure_merge cannot ever generate warning
                // conditions, because it shouldn't ever be run.
                r_sanity_check(conditions.size() == 0);
            }
            if (generated_keys_out->size() < limits.array_size_limit()) {
                generated_keys_out->push_back(key);
            } else {
                *keys_skipped_out += 1;
            }
        }
    }

    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> t = args->arg(env, 0)->as_table();
        return_changes_t return_changes = return_changes_t::NO;
        if (counted_t<val_t> v = args->optarg(env, "return_changes")) {
            return_changes = v->as_bool() ? return_changes_t::YES : return_changes_t::NO;
        }
        if (counted_t<val_t> v = args->optarg(env, "return_vals")) {
            rfail(base_exc_t::GENERIC, "return_vals renamed to return_changes");
        }

        const conflict_behavior_t conflict_behavior
            = parse_conflict_optarg(args->optarg(env, "conflict"), this);
        const durability_requirement_t durability_requirement
            = parse_durability_optarg(args->optarg(env, "durability"), this);

        bool done = false;
        counted_t<const datum_t> stats = new_stats_object();
        std::vector<std::string> generated_keys;
        std::set<std::string> conditions;
        size_t keys_skipped = 0;
        counted_t<val_t> v1 = args->arg(env, 1);
        if (v1->get_type().is_convertible(val_t::type_t::DATUM)) {
            std::vector<counted_t<const datum_t> > datums;
            datums.push_back(v1->as_datum());
            if (datums[0]->get_type() == datum_t::R_OBJECT) {
                try {
                    maybe_generate_key(t, env->env->limits(), &generated_keys,
                                       &keys_skipped, &datums[0]);
                } catch (const base_exc_t &) {
                    // We just ignore it, the same error will be handled in `replace`.
                    // TODO: that solution sucks.
                }
                counted_t<const datum_t> replace_stats = t->batched_insert(
                    env->env, std::move(datums), conflict_behavior,
                    durability_requirement, return_changes);
                stats = stats->merge(replace_stats, stats_merge, env->env->limits(), &conditions);
                done = true;
            }
        }

        if (!done) {
            counted_t<datum_stream_t> datum_stream = v1->as_seq(env->env);

            batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env->env);
            for (;;) {
                std::vector<counted_t<const datum_t> > datums
                    = datum_stream->next_batch(env->env, batchspec);
                if (datums.empty()) {
                    break;
                }

                for (auto it = datums.begin(); it != datums.end(); ++it) {
                    try {
                        maybe_generate_key(t, env->env->limits(),
                                           &generated_keys, &keys_skipped, &*it);
                    } catch (const base_exc_t &) {
                        // We just ignore it, the same error will be handled in
                        // `replace`.  TODO: that solution sucks.
                    }
                }

                counted_t<const datum_t> replace_stats = t->batched_insert(
                    env->env, std::move(datums), conflict_behavior, durability_requirement, return_changes);
                stats = stats->merge(replace_stats, stats_merge, env->env->limits(), &conditions);
            }
        }

        if (generated_keys.size() > 0) {
            std::vector<counted_t<const datum_t> > genkeys;
            genkeys.reserve(generated_keys.size());
            for (size_t i = 0; i < generated_keys.size(); ++i) {
                genkeys.push_back(make_counted<datum_t>(std::move(generated_keys[i])));
            }
            datum_object_builder_t d;
            UNUSED bool b = d.add("generated_keys",
                                  make_counted<datum_t>(std::move(genkeys),
                                                        env->env->limits()));
            stats = stats->merge(std::move(d).to_counted(), pure_merge,
                                 env->env->limits(), &conditions);
        }

        datum_object_builder_t obj(stats->as_object());
        obj.add_warnings(conditions, env->env->limits());
        if (keys_skipped > 0) {
            obj.add_warning(strprintf("Too many generated keys (%zu), array truncated to %zu.",
                                      keys_skipped + generated_keys.size(),
                                      generated_keys.size()).c_str(), env->env->limits());
        }

        return new_val(std::move(obj).to_counted());
    }
    virtual const char *name() const { return "insert"; }
};

class replace_term_t : public op_term_t {
public:
    replace_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2),
                    optargspec_t({"non_atomic", "durability", "return_vals", "return_changes"})) { }

private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        bool nondet_ok = false;
        if (counted_t<val_t> v = args->optarg(env, "non_atomic")) {
            nondet_ok = v->as_bool();
        }
        return_changes_t return_changes = return_changes_t::NO;
        if (counted_t<val_t> v = args->optarg(env, "return_changes")) {
            return_changes = v->as_bool() ? return_changes_t::YES : return_changes_t::NO;
        }
        if (counted_t<val_t> v = args->optarg(env, "return_vals")) {
            rfail(base_exc_t::GENERIC, "return_vals renamed to return_changes");
        }

        const durability_requirement_t durability_requirement
            = parse_durability_optarg(args->optarg(env, "durability"), this);

        counted_t<const func_t> f = args->arg(env, 1)->as_func(CONSTANT_SHORTCUT);
        if (!nondet_ok) {
            f->assert_deterministic("Maybe you want to use the non_atomic flag?");
        }

        counted_t<val_t> v0 = args->arg(env, 0);
        counted_t<const datum_t> stats = new_stats_object();
        std::set<std::string> conditions;
        if (v0->get_type().is_convertible(val_t::type_t::SINGLE_SELECTION)) {
            std::pair<counted_t<table_t>, counted_t<const datum_t> > tblrow
                = v0->as_single_selection();
            counted_t<const datum_t> orig_val = tblrow.second;
            counted_t<const datum_t> orig_key = v0->get_orig_key();
            if (!orig_key.has()) {
                orig_key = orig_val->get(tblrow.first->get_pkey(), NOTHROW);
                r_sanity_check(orig_key.has());
            }

            std::vector<counted_t<const datum_t> > vals;
            std::vector<counted_t<const datum_t> > keys;
            vals.push_back(orig_val);
            keys.push_back(orig_key);
            counted_t<const datum_t> replace_stats = tblrow.first->batched_replace(
                env->env, vals, keys, f,
                nondet_ok, durability_requirement, return_changes);
            stats = stats->merge(replace_stats, stats_merge, env->env->limits(),
                                 &conditions);
        } else {
            std::pair<counted_t<table_t>, counted_t<datum_stream_t> > tblrows
                = v0->as_selection(env->env);
            counted_t<table_t> tbl = tblrows.first;
            counted_t<datum_stream_t> ds = tblrows.second;

            batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env->env);
            for (;;) {
                std::vector<counted_t<const datum_t> > vals
                    = ds->next_batch(env->env, batchspec);
                if (vals.empty()) {
                    break;
                }
                std::vector<counted_t<const datum_t> > keys;
                keys.reserve(vals.size());
                for (auto it = vals.begin(); it != vals.end(); ++it) {
                    keys.push_back((*it)->get(tbl->get_pkey()));
                }
                counted_t<const datum_t> replace_stats = tbl->batched_replace(
                    env->env, vals, keys,
                    f, nondet_ok, durability_requirement, return_changes);
                stats = stats->merge(replace_stats, stats_merge, env->env->limits(), &conditions);
            }
        }

        datum_object_builder_t obj(stats->as_object());
        obj.add_warnings(conditions, env->env->limits());
        return new_val(std::move(obj).to_counted());
    }

    virtual const char *name() const { return "replace"; }
};

// DELETE and UPDATE are in rewrites.hpp

class foreach_term_t : public op_term_t {
public:
    foreach_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }

private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        const char *fail_msg = "FOREACH expects one or more basic write queries.";

        counted_t<datum_stream_t> ds = args->arg(env, 0)->as_seq(env->env);
        counted_t<const datum_t> stats = datum_t::empty_object();
        std::set<std::string> conditions;
        batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env->env);
        {
            profile::sampler_t sampler("Evaluating elements in for each.",
                                       env->env->trace);
            counted_t<const func_t> f = args->arg(env, 1)->as_func(CONSTANT_SHORTCUT);
            while (counted_t<const datum_t> row = ds->next(env->env, batchspec)) {
                counted_t<val_t> v = f->call(env->env, row);
                try {
                    counted_t<const datum_t> d = v->as_datum();
                    if (d->get_type() == datum_t::R_OBJECT) {
                        stats = stats->merge(d, stats_merge, env->env->limits(),
                                             &conditions);
                    } else {
                        for (size_t i = 0; i < d->size(); ++i) {
                            stats = stats->merge(d->get(i), stats_merge, env->env->limits(),
                                                 &conditions);
                        }
                    }
                } catch (const exc_t &e) {
                    throw exc_t(e.get_type(), fail_msg, e.backtrace());
                } catch (const datum_exc_t &de) {
                    rfail_target(v, base_exc_t::GENERIC, "%s  %s", fail_msg, de.what());
                }
                sampler.new_sample();
            }
        }
        datum_object_builder_t obj(stats->as_object());
        obj.add_warnings(conditions, env->env->limits());
        return new_val(std::move(obj).to_counted());
    }

    virtual const char *name() const { return "foreach"; }
};

counted_t<term_t> make_insert_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<insert_term_t>(env, term);
}
counted_t<term_t> make_replace_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<replace_term_t>(env, term);
}
counted_t<term_t> make_foreach_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<foreach_term_t>(env, term);
}

} // namespace ql
