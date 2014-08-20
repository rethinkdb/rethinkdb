// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <algorithm>
#include <map>
#include <string>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/math_utils.hpp"

namespace ql {

// TODO: Make this whole file not suck.

// Some Problems:
// * The method of constructing a canonical type from supertype * MAX_TYPE +
//   subtype is brittle.
// * Everything is done with nested ifs.  Ideally we'd build some sort of graph
//   structure and walk it.

static const int MAX_TYPE = 10;

static const int DB_TYPE = val_t::type_t::DB * MAX_TYPE;
static const int TABLE_TYPE = val_t::type_t::TABLE * MAX_TYPE;
static const int SELECTION_TYPE = val_t::type_t::SELECTION * MAX_TYPE;
static const int ARRAY_SELECTION_TYPE = SELECTION_TYPE + datum_t::R_ARRAY;
static const int SEQUENCE_TYPE = val_t::type_t::SEQUENCE * MAX_TYPE;
static const int SINGLE_SELECTION_TYPE = val_t::type_t::SINGLE_SELECTION * MAX_TYPE;
static const int DATUM_TYPE = val_t::type_t::DATUM * MAX_TYPE;
static const int FUNC_TYPE = val_t::type_t::FUNC * MAX_TYPE;
static const int GROUPED_DATA_TYPE = val_t::type_t::GROUPED_DATA * MAX_TYPE;

static const int R_NULL_TYPE = val_t::type_t::DATUM * MAX_TYPE + datum_t::R_NULL;
static const int R_BINARY_TYPE = val_t::type_t::DATUM * MAX_TYPE + datum_t::R_BINARY;
static const int R_BOOL_TYPE = val_t::type_t::DATUM * MAX_TYPE + datum_t::R_BOOL;
static const int R_NUM_TYPE = val_t::type_t::DATUM * MAX_TYPE + datum_t::R_NUM;
static const int R_STR_TYPE = val_t::type_t::DATUM * MAX_TYPE + datum_t::R_STR;
static const int R_ARRAY_TYPE = val_t::type_t::DATUM * MAX_TYPE + datum_t::R_ARRAY;
static const int R_OBJECT_TYPE = val_t::type_t::DATUM * MAX_TYPE + datum_t::R_OBJECT;

class coerce_map_t {
public:
    coerce_map_t() {
        map["DB"] = DB_TYPE;
        map["TABLE"] = TABLE_TYPE;
        map["SELECTION<STREAM>"] = SELECTION_TYPE;
        map["SELECTION<ARRAY>"] = ARRAY_SELECTION_TYPE;
        map["STREAM"] = SEQUENCE_TYPE;
        map["SELECTION<OBJECT>"] = SINGLE_SELECTION_TYPE;
        map["DATUM"] = DATUM_TYPE;
        map["FUNCTION"] = FUNC_TYPE;
        map["GROUPED_DATA"] = GROUPED_DATA_TYPE;
        CT_ASSERT(val_t::type_t::GROUPED_DATA < MAX_TYPE);

        map["NULL"] = R_NULL_TYPE;
        map["BINARY"] = R_BINARY_TYPE;
        map["BOOL"] = R_BOOL_TYPE;
        map["NUMBER"] = R_NUM_TYPE;
        map["STRING"] = R_STR_TYPE;
        map["ARRAY"] = R_ARRAY_TYPE;
        map["OBJECT"] = R_OBJECT_TYPE;
        CT_ASSERT(datum_t::R_STR < MAX_TYPE);

        for (std::map<std::string, int>::iterator
                 it = map.begin(); it != map.end(); ++it) {
            rmap[it->second] = it->first;
        }
        guarantee(map.size() == rmap.size());
    }
    int get_type(const std::string &s, const rcheckable_t *caller) const {
        std::map<std::string, int>::const_iterator it = map.find(s);
        rcheck_target(caller, base_exc_t::GENERIC, it != map.end(),
                      strprintf("Unknown Type: %s", s.c_str()));
        return it->second;
    }
    std::string get_name(int type) const {
        std::map<int, std::string>::const_iterator it = rmap.find(type);
        r_sanity_check(it != rmap.end());
        return it->second;
    }
private:
    std::map<std::string, int> map;
    std::map<int, std::string> rmap;

    // These functions are here so that if you add a new type you have to update
    // this file.
    // THINGS TO DO:
    // * Update the coerce_map_t
    // * Add the various coercions
    // * !!! CHECK WHETHER WE HAVE MORE THAN MAX_TYPE TYPES AND INCREASE !!!
    //   !!! MAX_TYPE IF WE DO                                           !!!
    void NOCALL_ct_catch_new_types(val_t::type_t::raw_type_t t, datum_t::type_t t2) {
        switch (t) {
        case val_t::type_t::DB:
        case val_t::type_t::TABLE:
        case val_t::type_t::SELECTION:
        case val_t::type_t::SEQUENCE:
        case val_t::type_t::SINGLE_SELECTION:
        case val_t::type_t::DATUM:
        case val_t::type_t::FUNC:
        case val_t::type_t::GROUPED_DATA:
        default: break;
        }
        switch (t2) {
        case datum_t::R_NULL:
        case datum_t::R_BINARY:
        case datum_t::R_BOOL:
        case datum_t::R_NUM:
        case datum_t::R_STR:
        case datum_t::R_ARRAY:
        case datum_t::R_OBJECT:
        default: break;
        }
    }
};

static const coerce_map_t _coerce_map;
static int get_type(std::string s, const rcheckable_t *caller) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return _coerce_map.get_type(s, caller);
}
static std::string get_name(int type) {
    return _coerce_map.get_name(type);
}

static val_t::type_t::raw_type_t supertype(int type) {
    return static_cast<val_t::type_t::raw_type_t>(type / MAX_TYPE);
}
static datum_t::type_t subtype(int type) {
    return static_cast<datum_t::type_t>(type - supertype(type));
}
static int merge_types(int supertype, int subtype) {
    return supertype * MAX_TYPE + subtype;
}

class coerce_term_t : public op_term_t {
public:
    coerce_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> val = args->arg(env, 0);
        val_t::type_t opaque_start_type = val->get_type();
        int start_supertype = opaque_start_type.raw_type;
        int start_subtype = 0;
        if (opaque_start_type.is_convertible(val_t::type_t::DATUM)) {
            start_supertype = val_t::type_t::DATUM;
            start_subtype = val->as_datum()->get_type();
        }
        int start_type = merge_types(start_supertype, start_subtype);

        std::string end_type_name = args->arg(env, 1)->as_str().to_std();
        int end_type = get_type(end_type_name, this);

        // Identity
        if ((!subtype(end_type)
             && opaque_start_type.is_convertible(supertype(end_type)))
            || start_type == end_type) {
            return val;
        }

        // DATUM -> *
        if (opaque_start_type.is_convertible(val_t::type_t::DATUM)) {
            counted_t<const datum_t> d = val->as_datum();
            // DATUM -> DATUM
            if (supertype(end_type) == val_t::type_t::DATUM) {
                if (start_type == R_BINARY_TYPE && end_type == R_STR_TYPE) {
                    scoped_ptr_t<wire_string_t> data =
                        wire_string_t::create_and_init(d->as_binary().size(),
                                                       d->as_binary().data());
                    return new_val(make_counted<const datum_t>(std::move(data)));
                }
                if (start_type == R_STR_TYPE && end_type == R_BINARY_TYPE) {
                    scoped_ptr_t<wire_string_t> data =
                        wire_string_t::create_and_init(d->as_str().size(),
                                                       d->as_str().data());
                    return new_val(datum_t::binary(std::move(data)));
                }

                // DATUM -> STR
                if (end_type == R_STR_TYPE) {
                    return new_val(make_counted<const datum_t>(d->print()));
                }

                // OBJECT -> ARRAY
                if (start_type == R_OBJECT_TYPE && end_type == R_ARRAY_TYPE) {
                    const std::map<std::string, counted_t<const datum_t> > &obj
                        = d->as_object();
                    std::vector<counted_t<const datum_t> > arr;
                    arr.reserve(obj.size());
                    for (auto it = obj.begin(); it != obj.end(); ++it) {
                        std::vector<counted_t<const datum_t> > pair;
                        pair.reserve(2);
                        pair.push_back(make_counted<datum_t>(std::string(it->first)));
                        pair.push_back(it->second);
                        arr.push_back(make_counted<datum_t>(std::move(pair),
                                                            env->env->limits()));
                    }
                    return new_val(make_counted<const datum_t>(std::move(arr),
                                                               env->env->limits()));
                }

                // STR -> NUM
                if (start_type == R_STR_TYPE && end_type == R_NUM_TYPE) {
                    const wire_string_t &s = d->as_str();
                    double dbl;
                    char end; // Used to ensure that there's no trailing garbage.
                    if (sscanf(s.c_str(), "%lf%c", &dbl, &end) == 1) {
                        return new_val(make_counted<const datum_t>(dbl));
                    } else {
                        rfail(base_exc_t::GENERIC, "Could not coerce `%s` to NUMBER.",
                              s.c_str());
                    }
                }
            }
            // TODO: Object to sequence?
        }

        if (opaque_start_type.is_convertible(val_t::type_t::SEQUENCE)
            && !(start_supertype == val_t::type_t::DATUM
                 && start_type != R_ARRAY_TYPE)) {
            counted_t<datum_stream_t> ds;
            try {
                ds = val->as_seq(env->env);
            } catch (const base_exc_t &e) {
                rfail(base_exc_t::GENERIC,
                      "Cannot coerce %s to %s (failed to produce intermediate stream).",
                      get_name(start_type).c_str(), get_name(end_type).c_str());
            }

            // SEQUENCE -> ARRAY
            if (end_type == R_ARRAY_TYPE || end_type == DATUM_TYPE) {
                return ds->to_array(env->env);
            }

            // SEQUENCE -> OBJECT
            if (start_type == R_ARRAY_TYPE && end_type == R_OBJECT_TYPE) {
                datum_object_builder_t obj;
                batchspec_t batchspec
                    = batchspec_t::user(batch_type_t::TERMINAL, env->env);
                {
                    profile::sampler_t sampler("Coercing to object.", env->env->trace);
                    while (auto pair = ds->next(env->env, batchspec)) {
                        std::string key = pair->get(0)->as_str().to_std();
                        counted_t<const datum_t> keyval = pair->get(1);
                        bool b = obj.add(key, keyval);
                        rcheck(!b, base_exc_t::GENERIC,
                               strprintf("Duplicate key `%s` in coerced object.  "
                                         "(got `%s` and `%s` as values)",
                                         key.c_str(),
                                         obj.at(key)->trunc_print().c_str(),
                                         keyval->trunc_print().c_str()));
                        sampler.new_sample();
                    }
                }
                return new_val(std::move(obj).to_counted());
            }
        }

        rfail_typed_target(val, "Cannot coerce %s to %s.",
                           get_name(start_type).c_str(), get_name(end_type).c_str());
    }
    virtual const char *name() const { return "coerce_to"; }
};

class ungroup_term_t : public op_term_t {
public:
    ungroup_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<grouped_data_t> groups
            = args->arg(env, 0)->as_promiscuous_grouped_data(env->env);
        std::vector<counted_t<const datum_t> > v;
        v.reserve(groups->size());

        iterate_ordered_by_version(
            env->env->reql_version(),
            *groups,
            [&v](const counted_t<const datum_t> &key, counted_t<const datum_t> &value) {
                r_sanity_check(key.has() && value.has());
                std::map<std::string, counted_t<const datum_t> > m =
                    {{"group", key},
                     {"reduction", std::move(value)}};
                v.push_back(make_counted<const datum_t>(std::move(m)));
            });
        return new_val(make_counted<const datum_t>(std::move(v), env->env->limits()));
    }
    virtual const char *name() const { return "ungroup"; }
    virtual bool can_be_grouped() const { return false; }
};

int val_type(counted_t<val_t> v) {
    int t = v->get_type().raw_type * MAX_TYPE;
    if (t == DATUM_TYPE) {
        t += v->as_datum()->get_type();
    } else if (t == SELECTION_TYPE) {
        if (v->sequence()->is_array()) {
            t += datum_t::R_ARRAY;
        }
    }
    return t;
}

class typeof_term_t : public op_term_t {
public:
    typeof_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> v = args->arg(env, 0);
        if (v->get_type().raw_type == val_t::type_t::DATUM) {
            counted_t<const datum_t> d = v->as_datum();
            return new_val(make_counted<const datum_t>(d->get_type_name()));
        } else if (v->get_type().raw_type == val_t::type_t::SEQUENCE
                   && v->as_seq(env->env)->is_grouped()) {
            return new_val(make_counted<const datum_t>("GROUPED_STREAM"));
        } else {
            return new_val(
                make_counted<const datum_t>(get_name(val_type(v))));
        }
    }
    virtual const char *name() const { return "typeof"; }
    virtual bool can_be_grouped() const { return false; }
};

class info_term_t : public op_term_t {
public:
    info_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        return new_val(val_info(env, args->arg(env, 0)));
    }

    counted_t<const datum_t> val_info(scope_env_t *env, counted_t<val_t> v) const {
        datum_object_builder_t info;
        int type = val_type(v);
        bool b = info.add("type", make_counted<datum_t>(get_name(type)));

        switch (type) {
        case DB_TYPE: {
            b |= info.add("name", make_counted<datum_t>(std::string(v->as_db()->name)));
        } break;
        case TABLE_TYPE: {
            counted_t<table_t> table = v->as_table();
            b |= info.add("name", make_counted<datum_t>(std::string(table->name)));
            b |= info.add("primary_key",
                          make_counted<datum_t>(std::string(table->get_pkey())));
            b |= info.add("indexes", table->sindex_list(env->env));
            b |= info.add("db", val_info(env, new_val(table->db)));
        } break;
        case SELECTION_TYPE: {
            b |= info.add("table",
                          val_info(env, new_val(v->as_selection(env->env).first)));
        } break;
        case ARRAY_SELECTION_TYPE: {
            b |= info.add("table",
                          val_info(env, new_val(v->as_selection(env->env).first)));
        } break;
        case SINGLE_SELECTION_TYPE: {
            b |= info.add("table",
                          val_info(env, new_val(v->as_single_selection().first)));
        } break;
        case SEQUENCE_TYPE: {
            if (v->as_seq(env->env)->is_grouped()) {
                bool res = info.add("type", make_counted<datum_t>("GROUPED_STREAM"));
                r_sanity_check(res);
            }
        } break;

        case FUNC_TYPE: {
            b |= info.add("source_code",
                          make_counted<datum_t>(v->as_func()->print_source()));
        } break;

        case GROUPED_DATA_TYPE: break; // No more info

        case R_BINARY_TYPE: // fallthru
            b |= info.add("count",
                          make_counted<datum_t>(
                              safe_to_double(v->as_datum()->as_binary().size())));

        case R_NULL_TYPE:   // fallthru
        case R_BOOL_TYPE:   // fallthru
        case R_NUM_TYPE:    // fallthru
        case R_STR_TYPE:    // fallthru
        case R_ARRAY_TYPE:  // fallthru
        case R_OBJECT_TYPE: // fallthru
        case DATUM_TYPE: {
            b |= info.add("value", make_counted<datum_t>(v->as_datum()->print()));
        } break;

        default: r_sanity_check(false);
        }
        r_sanity_check(!b);
        return std::move(info).to_counted();
    }

    virtual const char *name() const { return "info"; }
    virtual bool can_be_grouped() const { return false; }
};

counted_t<term_t> make_coerce_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<coerce_term_t>(env, term);
}
counted_t<term_t> make_ungroup_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<ungroup_term_t>(env, term);
}
counted_t<term_t> make_typeof_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<typeof_term_t>(env, term);
}
counted_t<term_t> make_info_term(
    compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<info_term_t>(env, term);
}

} // namespace ql
