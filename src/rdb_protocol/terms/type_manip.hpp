#include <algorithm>
#include <string>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"

namespace ql {

static const int MAX_TYPE = 10;

static const int DB_TYPE = val_t::type_t::DB * MAX_TYPE;
static const int TABLE_TYPE = val_t::type_t::TABLE * MAX_TYPE;
static const int SELECTION_TYPE = val_t::type_t::SELECTION * MAX_TYPE;
static const int SEQUENCE_TYPE = val_t::type_t::SEQUENCE * MAX_TYPE;
static const int SINGLE_SELECTION_TYPE = val_t::type_t::SINGLE_SELECTION * MAX_TYPE;
static const int DATUM_TYPE = val_t::type_t::DATUM * MAX_TYPE;
static const int FUNC_TYPE = val_t::type_t::FUNC * MAX_TYPE;

static const int R_NULL_TYPE = val_t::type_t::DATUM * MAX_TYPE + datum_t::R_NULL;
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
        map["SELECTION"] = SELECTION_TYPE;
        map["SEQUENCE"] = SEQUENCE_TYPE;
        map["SINGLE_SELECTION"] = SINGLE_SELECTION_TYPE;
        map["DATUM"] = DATUM_TYPE;
        map["FUNCTION"] = FUNC_TYPE;
        // TODO: CT_ASSERT this?
        r_sanity_check(val_t::type_t::FUNC < MAX_TYPE);

        map["NULL"] = R_NULL_TYPE;
        map["BOOL"] = R_BOOL_TYPE;
        map["NUMBER"] = R_NUM_TYPE;
        map["STRING"] = R_STR_TYPE;
        map["ARRAY"] = R_ARRAY_TYPE;
        map["OBJECT"] = R_OBJECT_TYPE;
        r_sanity_check(datum_t::R_OBJECT < MAX_TYPE);

        for (std::map<std::string, int>::iterator
                 it = map.begin(); it != map.end(); ++it) {
            rmap[it->second] = it->first;
        }
    }
    int get_type(const std::string &s) {
        rcheck(map.count(s) == 1, strprintf("Unknown Type: %s", s.c_str()));
        return map[s];
    }
    std::string get_name(int type) {
        r_sanity_check(rmap.count(type) == 1);
        return rmap[type];
    }
private:
    std::map<std::string, int> map;
    std::map<int, std::string> rmap;

    // These functions are here so that if you add a new type you have to update
    // this file.
    // THINGS TO DO:
    // * Update the coerce map
    // * Add the various coercions
    // * !!! CHECK WHETHER WE HAVE MORE THAN MAX_TYPE TYPES AND INCREASE !!!
    //   !!! MAX_TYPE IF WE DO                                           !!!
    void __ct_catch_new_types(val_t::type_t::raw_type_t t, datum_t::type_t t2) {
        switch(t) {
        case val_t::type_t::DB:
        case val_t::type_t::TABLE:
        case val_t::type_t::SELECTION:
        case val_t::type_t::SEQUENCE:
        case val_t::type_t::SINGLE_SELECTION:
        case val_t::type_t::DATUM:
        case val_t::type_t::FUNC:
        default:;
        }
        switch(t2) {
        case datum_t::R_NULL:
        case datum_t::R_BOOL:
        case datum_t::R_NUM:
        case datum_t::R_STR:
        case datum_t::R_ARRAY:
        case datum_t::R_OBJECT:
        default:;
        }
    }
};

static coerce_map_t _coerce_map;
static int get_type(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return _coerce_map.get_type(s);
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
    coerce_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        val_t *val = arg(0);
        val_t::type_t opaque_start_type = val->get_type();
        int start_supertype = opaque_start_type.raw_type;
        int start_subtype = 0;
        if (opaque_start_type.is_convertible(val_t::type_t::DATUM)) {
            start_subtype = val->as_datum()->get_type();
        }
        int start_type = merge_types(start_supertype, start_subtype);

        std::string end_type_name = arg(1)->as_datum()->as_str();
        int end_type = get_type(end_type_name);

        // Identity
        if (!subtype(end_type)
            && opaque_start_type.is_convertible(supertype(end_type))) {
            return val;
        }

        // DATUM -> *
        if (opaque_start_type.is_convertible(val_t::type_t::DATUM)) {
            const datum_t *d = val->as_datum();
            // DATUM -> DATUM
            if (supertype(end_type) == val_t::type_t::DATUM) {
                // DATUM -> STR
                if (end_type == R_STR_TYPE) {
                    return new_val(new datum_t(d->print()));
                }

                // OBJECT -> ARRAY
                if (start_type == R_OBJECT_TYPE && end_type == R_ARRAY_TYPE) {
                    datum_t *arr = env->add_ptr(new datum_t(datum_t::R_ARRAY));
                    const std::map<const std::string, const datum_t *> &obj =
                        d->as_object();
                    for (std::map<const std::string, const datum_t *>::const_iterator
                             it = obj.begin(); it != obj.end(); ++it) {
                        datum_t *pair = env->add_ptr(new datum_t(datum_t::R_ARRAY));
                        pair->add(env->add_ptr(new datum_t(it->first)));
                        pair->add(it->second);
                        arr->add(pair);
                    }
                    return new_val(arr);
                }

            }
            // TODO: Object to sequence?
        }

        if (opaque_start_type.is_convertible(val_t::type_t::SEQUENCE)) {
            datum_stream_t *ds = val->as_seq();
            // SEQUENCE -> ARRAY
            if (end_type == R_ARRAY_TYPE || end_type == DATUM_TYPE) {
                datum_t *arr = env->add_ptr(new datum_t(datum_t::R_ARRAY));
                while (const datum_t *el = ds->next()) arr->add(el);
                return new_val(arr);
            }

            // SEQUENCE -> OBJECT
            if (end_type == R_OBJECT_TYPE) {
                if (start_type == R_ARRAY_TYPE && end_type == R_OBJECT_TYPE) {
                    datum_t *obj = env->add_ptr(new datum_t(datum_t::R_OBJECT));
                    while (const datum_t *pair = ds->next()) {
                        std::string key = pair->el(0)->as_str();
                        const datum_t *keyval = pair->el(1);
                        bool b = obj->add(key, keyval);
                        rcheck(!b, strprintf("Duplicate key %s in coerced object. "
                                             " (got %s and %s as values)",
                                             key.c_str(),
                                             obj->el(key)->print().c_str(),
                                             keyval->print().c_str()));
                    }
                    return new_val(obj);
                }
            }

        }

        rfail("Cannot COERCE %s to %s.",
              get_name(start_type).c_str(), get_name(end_type).c_str());
        unreachable();
    }
    RDB_NAME("coerce")
};

class typeof_term_t : public op_term_t {
public:
    typeof_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual val_t *eval_impl() {
        int t = arg(0)->get_type().raw_type * MAX_TYPE;
        if (t == DATUM_TYPE) t += arg(0)->as_datum()->get_type();
        return new_val(get_name(t));
    }
    RDB_NAME("typeof")
};

} // namespace ql
