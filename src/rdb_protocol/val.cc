// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/val.hpp"

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/meta_utils.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/term.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

table_t::table_t(env_t *env,
                 counted_t<const db_t> _db, const std::string &_name,
                 bool _use_outdated, const protob_t<const Backtrace> &backtrace)
    : pb_rcheckable_t(backtrace),
      db(_db),
      name(_name),
      use_outdated(_use_outdated),
      sorting(UNORDERED) {
    uuid_u db_id = db->id;
    name_string_t table_name;
    bool b = table_name.assign_value(name);
    rcheck(b, base_exc_t::GENERIC,
           strprintf("Table name `%s` invalid (%s).",
                     name.c_str(), name_string_t::valid_char_msg));
    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >
        namespaces_metadata = env->cluster_access.namespaces_semilattice_metadata->get();
    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t
        namespaces_metadata_change(&namespaces_metadata);
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
        ns_searcher(&namespaces_metadata_change.get()->namespaces);
    // TODO: fold into iteration below
    namespace_predicate_t pred(&table_name, &db_id);
    uuid_u id = meta_get_uuid(&ns_searcher, pred,
                              strprintf("Table `%s` does not exist.",
                                        table_name.c_str()), this);

    access.init(new namespace_repo_t<rdb_protocol_t>::access_t(
                    env->cluster_access.ns_repo, id, env->interruptor));

    metadata_search_status_t status;
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >::iterator
        ns_metadata_it = ns_searcher.find_uniq(pred, &status);
    rcheck(status == METADATA_SUCCESS,
           base_exc_t::GENERIC,
           strprintf("Table `%s` does not exist.", table_name.c_str()));
    guarantee(!ns_metadata_it->second.is_deleted());
    r_sanity_check(!ns_metadata_it->second.get_ref().primary_key.in_conflict());
    pkey =  ns_metadata_it->second.get_ref().primary_key.get();
}

counted_t<const datum_t> table_t::make_error_datum(const base_exc_t &exception) {
    datum_ptr_t d(datum_t::R_OBJECT);
    std::string err = exception.what();

    // The bool is true if there's a conflict when inserting the
    // key, but since we just created an empty object above conflicts
    // are impossible here.  If you want to harden this against future
    // changes, you could store the bool and `r_sanity_check` that it's
    // false.
    DEBUG_VAR bool had_first_error
        = d.add("first_error", make_counted<datum_t>(std::move(err)));
    rassert(!had_first_error);

    DEBUG_VAR bool had_errors = d.add("errors", make_counted<datum_t>(1.0));
    rassert(!had_errors);

    return d.to_counted();
}

counted_t<const datum_t> table_t::replace(env_t *env,
                                          counted_t<const datum_t> original,
                                          counted_t<func_t> replacement_generator,
                                          bool nondet_ok,
                                          durability_requirement_t durability_requirement,
                                          bool return_vals) {
    try {
        return do_replace(env, original, replacement_generator, nondet_ok,
                          durability_requirement, return_vals);
    } catch (const base_exc_t &exc) {
        return make_error_datum(exc);
    }
}

counted_t<const datum_t> table_t::replace(env_t *env,
                                          counted_t<const datum_t> original,
                                          counted_t<const datum_t> replacement,
                                          bool upsert,
                                          durability_requirement_t durability_requirement,
                                          bool return_vals) {
    try {
        return do_replace(env, original, replacement, upsert,
                          durability_requirement, return_vals);
    } catch (const base_exc_t &exc) {
        return make_error_datum(exc);
    }
}

protob_t<Term> make_replacement_body(counted_t<const datum_t> replacement, UNUSED sym_t x) {
    protob_t<Term> twrap = make_counted_term();
    Term *const arg = twrap.get();
    replacement->write_to_protobuf(pb::set_datum(arg));
    return twrap;
}

std::vector<counted_t<const datum_t> > table_t::batch_replace(
        env_t *env,
        const std::vector<counted_t<const datum_t> > &original_values,
        counted_t<func_t> replacement_generator,
        const bool nondeterministic_replacements_ok,
        const durability_requirement_t durability_requirement) {
    if (replacement_generator->is_deterministic()) {
        std::vector<datum_func_pair_t> pairs(original_values.size());
        map_wire_func_t wire_func = map_wire_func_t(replacement_generator);
        for (size_t i = 0; i < original_values.size(); ++i) {
            try {
                pairs[i] = datum_func_pair_t(original_values[i], &wire_func);
            } catch (const base_exc_t &exc) {
                pairs[i] = datum_func_pair_t(make_error_datum(exc));
            }
        }

        return batch_replace(env, pairs, durability_requirement);
    } else {
        r_sanity_check(nondeterministic_replacements_ok);

        scoped_array_t<map_wire_func_t> funcs(original_values.size());
        std::vector<datum_func_pair_t> pairs(original_values.size());
        for (size_t i = 0; i < original_values.size(); ++i) {
            try {
                counted_t<const datum_t> replacement =
                    replacement_generator->call(env, original_values[i])->as_datum();

                funcs[i] = map_wire_func_t::make_safely(pb::dummy_var_t::IGNORED,
                                                        std::bind(make_replacement_body,
                                                                  replacement,
                                                                  std::placeholders::_1),
                                                        backtrace());
                pairs[i] = datum_func_pair_t(original_values[i], &funcs[i]);
            } catch (const base_exc_t &exc) {
                pairs[i] = datum_func_pair_t(make_error_datum(exc));
            }
        }

        return batch_replace(env, pairs, durability_requirement);
    }
}

protob_t<Term> make_upsert_replace_body(bool upsert, counted_t<const datum_t> d, sym_t x) {
    protob_t<Term> twrap = make_counted_term();
    Term *const arg = twrap.get();
    if (upsert) {
        d->write_to_protobuf(pb::set_datum(arg));
    } else {
        N3(BRANCH,
           N2(EQ, NVAR(x), NDATUM(datum_t::R_NULL)),
           NDATUM(d),
           N1(ERROR, NDATUM("Duplicate primary key.")));
    }
    return twrap;
}

map_wire_func_t upsert_replacement_func(bool upsert, counted_t<const datum_t> d,
                                        protob_t<const Backtrace> backtrace) {
    return map_wire_func_t::make_safely(pb::dummy_var_t::VAL_UPSERT_REPLACEMENT,
                                        std::bind(make_upsert_replace_body,
                                                  upsert, d,
                                                  std::placeholders::_1),
                                        backtrace);
}

std::vector<counted_t<const datum_t> > table_t::batch_replace(
        env_t *env,
        const std::vector<counted_t<const datum_t> > &original_values,
        const std::vector<counted_t<const datum_t> > &replacement_values,
        const bool upsert,
        const durability_requirement_t durability_requirement) {
    r_sanity_check(original_values.size() == replacement_values.size());
    scoped_array_t<map_wire_func_t> funcs(original_values.size());
    std::vector<datum_func_pair_t> pairs(original_values.size());
    for (size_t i = 0; i < original_values.size(); ++i) {
        try {
            funcs[i] = upsert_replacement_func(upsert, replacement_values[i], backtrace());
            pairs[i] = datum_func_pair_t(original_values[i], &funcs[i]);
        } catch (const base_exc_t &exc) {
            pairs[i] = datum_func_pair_t(make_error_datum(exc));
        }
    }

    return batch_replace(env, pairs, durability_requirement);
}

bool is_sorted_by_first(const std::vector<std::pair<int64_t, Datum> > &v) {
    if (v.size() == 0) {
        return true;
    }

    auto it = v.begin();
    auto jt = it + 1;
    while (jt < v.end()) {
        if (!(it->first < jt->first)) {
            return false;
        }
        ++it;
        ++jt;
    }
    return true;
}


std::vector<counted_t<const datum_t> > table_t::batch_replace(
        env_t *env,
        const std::vector<datum_func_pair_t> &replacements,
        const durability_requirement_t durability_requirement) {
    std::vector<counted_t<const datum_t> > ret(replacements.size());

    std::vector<std::pair<int64_t, rdb_protocol_t::point_replace_t> > point_replaces;

    for (size_t i = 0; i < replacements.size(); ++i) {
        try {
            if (replacements[i].error_value.has()) {
                r_sanity_check(!replacements[i].original_value.has());
                ret[i] = replacements[i].error_value;
            } else {
                counted_t<const datum_t> orig = replacements[i].original_value;
                r_sanity_check(orig.has());

                if (orig->get_type() == datum_t::R_NULL) {
                    // TODO: We copy this for some reason, possibly no reason.
                    map_wire_func_t mwf = *replacements[i].replacer;
                    orig = mwf.compile_wire_func()->call(env, orig)->as_datum();
                    if (orig->get_type() == datum_t::R_NULL) {
                        datum_ptr_t resp(datum_t::R_OBJECT);
                        counted_t<const datum_t> one(new datum_t(1.0));
                        const bool b = resp.add("skipped", one);
                        r_sanity_check(!b);
                        ret[i] = resp.to_counted();
                        continue;
                    }
                }

                const std::string &pk = get_pkey();
                store_key_t store_key(orig->get(pk)->print_primary());
                point_replaces.push_back(
                    std::make_pair(static_cast<int64_t>(point_replaces.size()),
                                   rdb_protocol_t::point_replace_t(
                                       pk, store_key,
                                       *replacements[i].replacer,
                                       env->global_optargs.get_all_optargs(),
                                       false)));
            }
        } catch (const base_exc_t& exc) {
            ret[i] = make_error_datum(exc);
        }
    }

    if (!point_replaces.empty()) {
        rdb_protocol_t::write_t write(rdb_protocol_t::batched_replaces_t(point_replaces),
                                      durability_requirement);
        rdb_protocol_t::write_response_t response;
        access->get_namespace_if()->write(write, &response, order_token_t::ignore, env->interruptor);
        rdb_protocol_t::batched_replaces_response_t *batched_replaces_response
            = boost::get<rdb_protocol_t::batched_replaces_response_t>(&response.response);
        r_sanity_check(batched_replaces_response != NULL);
        std::vector<std::pair<int64_t, Datum> > *datums = &batched_replaces_response->point_replace_responses;

        rassert(is_sorted_by_first(*datums));

        size_t j = 0;
        for (size_t i = 0; i < ret.size(); ++i) {
            if (!ret[i].has()) {
                r_sanity_check(j < datums->size());
                ret[i] = make_counted<datum_t>(&(*datums)[j].second);
                ++j;
            }
        }
        r_sanity_check(j == datums->size());
    }

    return ret;
}

MUST_USE bool table_t::sindex_create(env_t *env,
                                     const std::string &id,
                                     counted_t<func_t> index_func,
                                     sindex_multi_bool_t multi) {
    index_func->assert_deterministic("Index functions must be deterministic.");
    map_wire_func_t wire_func(index_func);
    rdb_protocol_t::write_t write(
            rdb_protocol_t::sindex_create_t(id, wire_func, multi));

    rdb_protocol_t::write_response_t res;
    access->get_namespace_if()->write(
        write, &res, order_token_t::ignore, env->interruptor);

    rdb_protocol_t::sindex_create_response_t *response =
        boost::get<rdb_protocol_t::sindex_create_response_t>(&res.response);
    r_sanity_check(response);
    return response->success;
}

MUST_USE bool table_t::sindex_drop(env_t *env, const std::string &id) {
    rdb_protocol_t::write_t write((
            rdb_protocol_t::sindex_drop_t(id)));

    rdb_protocol_t::write_response_t res;
    access->get_namespace_if()->write(
        write, &res, order_token_t::ignore, env->interruptor);

    rdb_protocol_t::sindex_drop_response_t *response =
        boost::get<rdb_protocol_t::sindex_drop_response_t>(&res.response);
    r_sanity_check(response);
    return response->success;
}

counted_t<const datum_t> table_t::sindex_list(env_t *env) {
    rdb_protocol_t::sindex_list_t sindex_list;
    rdb_protocol_t::read_t read(sindex_list);
    try {
        rdb_protocol_t::read_response_t res;
        access->get_namespace_if()->read(
            read, &res, order_token_t::ignore, env->interruptor);
        rdb_protocol_t::sindex_list_response_t *s_res =
            boost::get<rdb_protocol_t::sindex_list_response_t>(&res.response);
        r_sanity_check(s_res);

        std::vector<counted_t<const datum_t> > array;
        array.reserve(s_res->sindexes.size());

        for (std::vector<std::string>::const_iterator it = s_res->sindexes.begin();
             it != s_res->sindexes.end(); ++it) {
            array.push_back(make_counted<datum_t>(std::string(*it)));
        }
        return make_counted<datum_t>(std::move(array));

    } catch (const cannot_perform_query_exc_t &ex) {
        rfail(ql::base_exc_t::GENERIC, "cannot perform read: %s", ex.what());
    }
}

counted_t<const datum_t> table_t::do_replace(
    env_t *env,
    counted_t<const datum_t> orig,
    const map_wire_func_t &mwf,
    durability_requirement_t durability_requirement,
    bool return_vals) {
    const std::string &pk = get_pkey();
    if (orig->get_type() == datum_t::R_NULL) {
        map_wire_func_t mwf2 = mwf;
        orig = mwf2.compile_wire_func()->call(env, orig)->as_datum();
        if (orig->get_type() == datum_t::R_NULL) {
            datum_ptr_t resp(datum_t::R_OBJECT);
            bool b = resp.add("skipped", make_counted<datum_t>(1.0));
            r_sanity_check(!b);
            return resp.to_counted();
        }
    }
    store_key_t store_key(orig->get(pk)->print_primary());
    rdb_protocol_t::write_t write(
        rdb_protocol_t::point_replace_t(
            pk, store_key, mwf, env->global_optargs.get_all_optargs(), return_vals),
        durability_requirement);

    rdb_protocol_t::write_response_t response;
    access->get_namespace_if()->write(
        write, &response, order_token_t::ignore, env->interruptor);
    Datum *d = boost::get<Datum>(&response.response);
    return make_counted<datum_t>(d);
}

counted_t<const datum_t> table_t::do_replace(env_t *env,
                                             counted_t<const datum_t> orig,
                                             counted_t<func_t> f,
                                             bool nondet_ok,
                                             durability_requirement_t durability_requirement,
                                             bool return_vals) {
    if (f->is_deterministic()) {
        return do_replace(env, orig, map_wire_func_t(f),
                          durability_requirement, return_vals);
    } else {
        r_sanity_check(nondet_ok);
        return do_replace(env, orig, f->call(env, orig)->as_datum(), true,
                          durability_requirement, return_vals);
    }
}

counted_t<const datum_t> table_t::do_replace(env_t *env,
                                             counted_t<const datum_t> orig,
                                             counted_t<const datum_t> d,
                                             bool upsert,
                                             durability_requirement_t durability_requirement,
                                             bool return_vals) {
    map_wire_func_t func = upsert_replacement_func(upsert, d, backtrace());

    return do_replace(env, orig, func,
                      durability_requirement, return_vals);
}

const std::string &table_t::get_pkey() { return pkey; }

counted_t<const datum_t> table_t::get_row(env_t *env, counted_t<const datum_t> pval) {
    std::string pks = pval->print_primary();
    rdb_protocol_t::read_t read((rdb_protocol_t::point_read_t(store_key_t(pks))));
    rdb_protocol_t::read_response_t res;
    if (use_outdated) {
        access->get_namespace_if()->read_outdated(read, &res, env->interruptor);
    } else {
        access->get_namespace_if()->read(
            read, &res, order_token_t::ignore, env->interruptor);
    }
    rdb_protocol_t::point_read_response_t *p_res =
        boost::get<rdb_protocol_t::point_read_response_t>(&res.response);
    r_sanity_check(p_res);
    return p_res->data;
}

counted_t<datum_stream_t> table_t::get_all(
        env_t *env,
        counted_t<const datum_t> value,
        const std::string &get_all_sindex_id,
        const protob_t<const Backtrace> &bt) {
    rcheck_src(bt.get(), base_exc_t::GENERIC, !sindex_id,
            "Cannot chain get_all and other indexed operations.");
    r_sanity_check(sorting == UNORDERED);
    r_sanity_check(!bounds);

    if (get_all_sindex_id == get_pkey()) {
        return make_counted<lazy_datum_stream_t>(
                env, use_outdated, access.get(),
                value, false,
                value, false,
                UNORDERED, bt);
    } else {
        return make_counted<lazy_datum_stream_t>(
                env, use_outdated, access.get(),
                value, false,
                value, false,
                get_all_sindex_id, UNORDERED, bt);
    }
}

void table_t::add_sorting(const std::string &new_sindex_id, sorting_t _sorting,
                          const rcheckable_t *parent) {
    r_sanity_check(_sorting != UNORDERED);

    rcheck_target(parent, base_exc_t::GENERIC, !sorting,
            "Cannot apply 2 indexed orderings to the same TABLE.");
    rcheck_target(parent, base_exc_t::GENERIC, !sindex_id || *sindex_id == new_sindex_id,
            strprintf(
                "Cannot use 2 indexes in the same operation. Trying to use %s and %s",
                sindex_id->c_str(), new_sindex_id.c_str()));

    sindex_id = new_sindex_id;
    sorting = _sorting;
}

void table_t::add_bounds(
    counted_t<const datum_t> left_bound, bool left_bound_open,
    counted_t<const datum_t> right_bound, bool right_bound_open,
    const std::string &new_sindex_id, const rcheckable_t *parent) {

    if (sindex_id) {
        rcheck_target(parent, base_exc_t::GENERIC, *sindex_id == new_sindex_id,
            strprintf(
                "Cannot use 2 indexes in the same operation. Trying to use %s and %s",
                sindex_id->c_str(), new_sindex_id.c_str()));
    }

    rcheck_target(parent, base_exc_t::GENERIC, !bounds,
            "Cannot chain multiple betweens to the same table.");

    sindex_id = new_sindex_id;
    bounds = std::make_pair(
            bound_t(left_bound, left_bound_open),
            bound_t(right_bound, right_bound_open));
}

counted_t<datum_stream_t> table_t::as_datum_stream(env_t *env,
                                                   const protob_t<const Backtrace> &bt) {
    if (!sindex_id || *sindex_id == get_pkey()) {
        if (bounds) {
            return make_counted<lazy_datum_stream_t>(
                env, use_outdated, access.get(),
                bounds->first.value, bounds->first.bound_open,
                bounds->second.value, bounds->second.bound_open,
                sorting, bt);
        } else {
            return make_counted<lazy_datum_stream_t>(
                env, use_outdated, access.get(),
                sorting, bt);
        }
    } else {
        if (bounds) {
            return make_counted<lazy_datum_stream_t>(
                env, use_outdated, access.get(),
                bounds->first.value, bounds->first.bound_open,
                bounds->second.value, bounds->second.bound_open,
                *sindex_id, sorting, bt);
        } else {
            return make_counted<lazy_datum_stream_t>(
                env, use_outdated, access.get(),
                *sindex_id, sorting, bt);
        }
    }
}

val_t::type_t::type_t(val_t::type_t::raw_type_t _raw_type) : raw_type(_raw_type) { }

// NOTE: This *MUST* be kept in sync with the surrounding code (not that it
// should have to change very often).
bool raw_type_is_convertible(val_t::type_t::raw_type_t _t1,
                             val_t::type_t::raw_type_t _t2) {
    const int t1 = _t1, t2 = _t2,
        DB = val_t::type_t::DB,
        TABLE = val_t::type_t::TABLE,
        SELECTION = val_t::type_t::SELECTION,
        SEQUENCE = val_t::type_t::SEQUENCE,
        SINGLE_SELECTION = val_t::type_t::SINGLE_SELECTION,
        DATUM = val_t::type_t::DATUM,
        FUNC = val_t::type_t::FUNC;
    switch (t1) {
    case DB: return t2 == DB;
    case TABLE: return t2 == TABLE || t2 == SELECTION || t2 == SEQUENCE;
    case SELECTION: return t2 == SELECTION || t2 == SEQUENCE;
    case SEQUENCE: return t2 == SEQUENCE;
    case SINGLE_SELECTION: return t2 == SINGLE_SELECTION || t2 == DATUM;
    case DATUM: return t2 == DATUM || t2 == SEQUENCE;
    case FUNC: return t2 == FUNC;
    default: unreachable();
    }
}
bool val_t::type_t::is_convertible(type_t rhs) const {
    return raw_type_is_convertible(raw_type, rhs.raw_type);
}

const char *val_t::type_t::name() const {
    switch (raw_type) {
    case DB: return "DATABASE";
    case TABLE: return "TABLE";
    case SELECTION: return "SELECTION";
    case SEQUENCE: return "SEQUENCE";
    case SINGLE_SELECTION: return "SINGLE_SELECTION";
    case DATUM: return "DATUM";
    case FUNC: return "FUNCTION";
    default: unreachable();
    }
}

val_t::val_t(counted_t<const datum_t> _datum, protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::DATUM),
      u(_datum) {
    guarantee(datum().has());
}

val_t::val_t(counted_t<const datum_t> _datum, counted_t<table_t> _table,
             protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::SINGLE_SELECTION),
      table(_table),
      u(_datum) {
    guarantee(table.has());
    guarantee(datum().has());
}

val_t::val_t(env_t *env, counted_t<datum_stream_t> _sequence,
             protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::SEQUENCE),
      u(_sequence) {
    guarantee(sequence().has());
    // Some streams are really arrays in disguise.
    counted_t<const datum_t> arr = sequence()->as_array(env);
    if (arr.has()) {
        type = type_t::DATUM;
        u = arr;
    }
}

val_t::val_t(counted_t<table_t> _table,
             counted_t<datum_stream_t> _sequence,
             protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::SELECTION),
      table(_table),
      u(_sequence) {
    guarantee(table.has());
    guarantee(sequence().has());
}

val_t::val_t(counted_t<table_t> _table, protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::TABLE),
      table(_table) {
    guarantee(table.has());
}
val_t::val_t(counted_t<const db_t> _db, protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::DB),
      u(_db) {
    guarantee(db().has());
}
val_t::val_t(counted_t<func_t> _func, protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::FUNC),
      u(_func) {
    guarantee(func().has());
}

val_t::~val_t() { }

val_t::type_t val_t::get_type() const { return type; }
const char * val_t::get_type_name() const { return get_type().name(); }

counted_t<const datum_t> val_t::as_datum() const {
    if (type.raw_type != type_t::DATUM && type.raw_type != type_t::SINGLE_SELECTION) {
        rcheck_literal_type(type_t::DATUM);
    }
    return datum();
}

counted_t<table_t> val_t::as_table() {
    rcheck_literal_type(type_t::TABLE);
    return table;
}

counted_t<datum_stream_t> val_t::as_seq(env_t *env) {
    if (type.raw_type == type_t::SEQUENCE || type.raw_type == type_t::SELECTION) {
        return sequence();
    } else if (type.raw_type == type_t::TABLE) {
        return table->as_datum_stream(env, backtrace());
    } else if (type.raw_type == type_t::DATUM) {
        return datum()->as_datum_stream(backtrace());
    }
    rcheck_literal_type(type_t::SEQUENCE);
    unreachable();
}

std::pair<counted_t<table_t>, counted_t<datum_stream_t> > val_t::as_selection(env_t *env) {
    if (type.raw_type != type_t::TABLE && type.raw_type != type_t::SELECTION) {
        rcheck_literal_type(type_t::SELECTION);
    }
    return std::make_pair(table, as_seq(env));
}

std::pair<counted_t<table_t>, counted_t<const datum_t> > val_t::as_single_selection() {
    rcheck_literal_type(type_t::SINGLE_SELECTION);
    return std::make_pair(table, datum());
}

counted_t<func_t> val_t::as_func(function_shortcut_t shortcut) {
    if (get_type().is_convertible(type_t::FUNC)) {
        r_sanity_check(func().has());
        return func();
    }

    if (shortcut == NO_SHORTCUT) {
        rcheck_literal_type(type_t::FUNC);
        unreachable();
    }

    if (!type.is_convertible(type_t::DATUM)) {
        rcheck_literal_type(type_t::FUNC);
        unreachable();
    }

    // We use a switch here so that people have to update it if they add another
    // shortcut.
    switch (shortcut) {
    case CONSTANT_SHORTCUT:
        return new_constant_func(as_datum(), backtrace());
    case GET_FIELD_SHORTCUT:
        return new_get_field_func(as_datum(), backtrace());
    case PLUCK_SHORTCUT:
        return new_pluck_func(as_datum(), backtrace());
    case NO_SHORTCUT:
        // fallthru
    default: unreachable();
    }
}

counted_t<const db_t> val_t::as_db() const {
    rcheck_literal_type(type_t::DB);
    return db();
}

counted_t<const datum_t> val_t::as_ptype(const std::string s) {
    try {
        counted_t<const datum_t> d = as_datum();
        r_sanity_check(d.has());
        d->rcheck_is_ptype(s);
        return d;
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}

bool val_t::as_bool() {
    try {
        counted_t<const datum_t> d = as_datum();
        r_sanity_check(d.has());
        return d->as_bool();
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}
double val_t::as_num() {
    try {
        counted_t<const datum_t> d = as_datum();
        r_sanity_check(d.has());
        return d->as_num();
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}
int64_t val_t::as_int() {
    try {
        counted_t<const datum_t> d = as_datum();
        r_sanity_check(d.has());
        return d->as_int();
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}
const std::string &val_t::as_str() {
    try {
        counted_t<const datum_t> d = as_datum();
        r_sanity_check(d.has());
        return d->as_str();
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}

void val_t::rcheck_literal_type(type_t::raw_type_t expected_raw_type) const {
    rcheck_typed_target(
        this, type.raw_type == expected_raw_type,
        strprintf("Expected type %s but found %s:\n%s",
                  type_t(expected_raw_type).name(), type.name(), print().c_str()));
}

std::string val_t::print() const {
    if (get_type().is_convertible(type_t::DATUM)) {
        return as_datum()->print();
    } else if (get_type().is_convertible(type_t::DB)) {
        return strprintf("db(\"%s\")", as_db()->name.c_str());
    } else if (get_type().is_convertible(type_t::TABLE)) {
        return strprintf("table(\"%s\")", table->name.c_str());
    } else if (get_type().is_convertible(type_t::SELECTION)) {
        return strprintf("OPAQUE SELECTION ON table(%s)",
                         table->name.c_str());
    } else {
        // TODO: Do something smarter here?
        return strprintf("OPAQUE VALUE %s", get_type().name());
    }
}

std::string val_t::trunc_print() const {
    if (get_type().is_convertible(type_t::DATUM)) {
        return as_datum()->trunc_print();
    } else {
        std::string s = print();
        if (s.size() > datum_t::trunc_len) {
            s.erase(s.begin() + (datum_t::trunc_len - 3), s.end());
            s += "...";
        }
        return s;
    }
}


} // namespace ql
