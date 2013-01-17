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
        map["FUNC"] = FUNC_TYPE;
        // TODO: CT_ASSERT this?
        r_sanity_check(val_t::type_t::FUNC < MAX_TYPE);

        map["NULL"] = R_NULL_TYPE;
        map["BOOL"] = R_BOOL_TYPE;
        map["NUM"] = R_NUM_TYPE;
        map["STR"] = R_STR_TYPE;
        map["STRING"] = R_STR_TYPE;
        map["ARR"] = R_ARRAY_TYPE;
        map["ARRAY"] = R_ARRAY_TYPE;
        map["OBJ"] = R_OBJECT_TYPE;
        map["OBJECT"] = R_OBJECT_TYPE;
        r_sanity_check(datum_t::R_OBJECT < MAX_TYPE);
    }
    int get_type(const std::string &s) {
        if (map.count(s) == 0) return -1;
        return map[s];
    }
private:
    std::map<std::string, int> map;

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
    int i = _coerce_map.get_type(s);
    rcheck(i != -1, strprintf("Unknown Type: %s", s.c_str()));
    return i;
}


#define TPAIR(TYPE1, TYPE2) (((TYPE1)*MAX_TYPE*MAX_TYPE)+(TYPE2))
class coerce_term_t : public op_term_t {
public:
    coerce_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual val_t *eval_impl() {
        val_t *val = arg(0);
        int start_type = val->get_type().raw_type * MAX_TYPE;
        if (start_type == DATUM_TYPE) start_type += val->as_datum()->get_type();
        std::string end_type_name = arg(1)->as_datum()->as_str();
        int end_type = get_type(end_type_name);

        switch(TPAIR(start_type, end_type)) {
        case TPAIR(TABLE_TYPE, DATUM_TYPE): // fallthru
        case TPAIR(SEQUENCE_TYPE, DATUM_TYPE): // fallthru
        case TPAIR(TABLE_TYPE, R_ARRAY_TYPE): // fallthru
        case TPAIR(SEQUENCE_TYPE, R_ARRAY_TYPE): {
            scoped_ptr_t<datum_t> d(new datum_t(datum_t::R_ARRAY));
            datum_stream_t *ds = arg(0)->as_seq();
            while (const datum_t *el = ds->next()) d->add(el);
            return new_val(d.release());
        }; break;
        default: {
            rfail("Cannot COERCE %s to %s.",
                  val->get_type().name(),
                  end_type_name.c_str());
        }; break;
        }
        unreachable();
    }
    RDB_NAME("coerce")
};

} // namespace ql
