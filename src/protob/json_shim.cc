#include <inttypes.h>
#include "protob/json_shim.hpp"

#include "utils.hpp"
#include "debug.hpp"

#include "http/json.hpp"
#include "rdb_protocol/ql2.pb.h"

std::map<std::string, int32_t> resolver;

namespace json_shim {

class exc_t : public std::exception {
public:
    exc_t() { }
    ~exc_t() throw () { }
    const char *what() const throw () { return "json_shim::exc_t"; }
};

void check_type(cJSON *json, int expected) {
    if (json->type != expected) throw exc_t();
}

template<class T>
typename std::enable_if<!((std::is_enum<T>::value || std::is_fundamental<T>::value)
                          && !std::is_same<T, bool>::value)>::type
extract(cJSON *, T *);

template<class T>
typename std::enable_if<(std::is_enum<T>::value || std::is_fundamental<T>::value)
                        && !std::is_same<T, bool>::value>::type
extract(cJSON *field, T *dest) {
    if (field->type != cJSON_Number) throw exc_t();
    T t = static_cast<T>(field->valuedouble);
    if (static_cast<double>(t) != field->valuedouble) throw exc_t();
    *dest = t;
}

template<class T>
void safe_extract(cJSON *json, T *t) {
    if (json != NULL && t != NULL) {
        extract(json, t);
    }
}

template<class T, class U>
void transfer(cJSON *json, T *dest, void (T::*setter)(U), const char *name) {
    U tmp;
    if (cJSON *item = cJSON_GetObjectItem(json, name)) {
        safe_extract(item, &tmp);
        (dest->*setter)(std::move(tmp));
    }
}

template<class T, class U>
void transfer(cJSON *json, T *dest, U *(T::*mut)(), const char *name) {
    if (cJSON *item = cJSON_GetObjectItem(json, name)) {
        safe_extract(item, (dest->*mut)());
    }
}

template<class T, class U>
void transfer_arr(cJSON *json, T *dest, U *(T::*adder)(), const char *name) {
    if (cJSON *arr = cJSON_GetObjectItem(json, name)) {
        if (arr->type != cJSON_Object) {
            check_type(arr, cJSON_Array);
        }
        int sz = cJSON_GetArraySize(arr);
        for (int i = 0; i < sz; ++i) {
            safe_extract(cJSON_GetArrayItem(arr, i), (dest->*adder)());
        }
    }
}

template<>
void extract(cJSON *field, std::string *s) {
    check_type(field, cJSON_String);
    *s = field->valuestring;
}

template<>
void extract(cJSON *field, bool *dest) {
    if (field->type == cJSON_False) {
        *dest = false;
    } else if (field->type == cJSON_True) {
        *dest = true;
    } else {
        throw exc_t();
    }
}

template<>
void extract(cJSON *json, Term *t) {
    transfer(json, t, &Term::set_type, "t");
    transfer(json, t, &Term::mutable_datum, "d");
    transfer_arr(json, t, &Term::add_args, "a");
    transfer_arr(json, t, &Term::add_optargs, "o");
}

template<>
void extract(cJSON *json, Datum *d) {
    switch (json->type) {
    case cJSON_False: {
        d->set_type(Datum::R_BOOL);
        d->set_r_bool(false);
    } break;
    case cJSON_True: {
        d->set_type(Datum::R_BOOL);
        d->set_r_bool(true);
    } break;
    case cJSON_NULL: {
        d->set_type(Datum::R_NULL);
    } break;
    case cJSON_Number: {
        d->set_type(Datum::R_NUM);
        d->set_r_num(json->valuedouble);
    } break;
    case cJSON_String: {
        d->set_type(Datum::R_STR);
        d->set_r_str(json->valuestring);
    } break;
    case cJSON_Array: {
        d->set_type(Datum::R_ARRAY);
        int sz = cJSON_GetArraySize(json);
        for (int i = 0; i < sz; ++i) {
            cJSON *item = cJSON_GetArrayItem(json, i);
            extract(item, d->add_r_array());
        }
    } break;
    case cJSON_Object: {
        d->set_type(Datum::R_OBJECT);
        int sz = cJSON_GetArraySize(json);
        for (int i = 0; i < sz; ++i) {
            cJSON *item = cJSON_GetArrayItem(json, i);
            Datum::AssocPair *ap = d->add_r_object();
            ap->set_key(item->string);
            extract(item, ap->mutable_val());
        }
    } break;
    default: unreachable();
    }
}

template<>
void extract(cJSON *json, Query::AssocPair *ap) {
    ap->set_key(json->string);
    extract(json, ap->mutable_val());
}

template<>
void extract(cJSON *json, Term::AssocPair *ap) {
    ap->set_key(json->string);
    extract(json, ap->mutable_val());
}

template<>
void extract(cJSON *json, Datum::AssocPair *ap) {
    ap->set_key(json->string);
    extract(json, ap->mutable_val());
}

template<>
void extract(cJSON *json, Query *q) {
    transfer(json, q, &Query::set_type, "t");
    transfer(json, q, &Query::mutable_query, "q");
    transfer(json, q, &Query::set_token, "k");
    q->set_accepts_r_json(true);
    transfer_arr(json, q, &Query::add_global_optargs, "g");
}

bool parse_json_pb(Query *q, char *str) throw () {
    q->Clear();
    scoped_cJSON_t json_holder(cJSON_Parse(str));
    cJSON *json = json_holder.get();
    // debugf("%s\n", json_holder.Print().c_str());
    if (json == NULL) return false;
    extract(json, q);
    // debugf("%s\n", q->DebugString().c_str());
    return true;
}

int64_t write_json_pb(const Response *r, std::string *s) throw () {
    *s += strprintf("{\"t\":%d,\"k\":%" PRIi64 ",\"r\":[", r->type(), r->token());
    for (int i = 0; i < r->response_size(); ++i) {
        *s += (i == 0) ? "" : ",";
        const Datum *d = &r->response(i);
        if (d->type() == Datum::R_JSON) {
            *s += d->r_str();
        } else if (d->type() == Datum::R_STR) {
            scoped_cJSON_t tmp(cJSON_CreateString(d->r_str().c_str()));
            *s += tmp.PrintUnformatted();
        } else {
            unreachable();
        }
    }
    *s += "]";

    if (r->has_backtrace()) {
        *s += ",\"b\":";
        const Backtrace *bt = &r->backtrace();
        scoped_cJSON_t arr(cJSON_CreateArray());
        for (int i = 0; i < bt->frames_size(); ++i) {
            const Frame *f = &bt->frames(i);
            switch (f->type()) {
            case Frame::POS: {
                arr.AddItemToArray(cJSON_CreateNumber(f->pos()));
            } break;
            case Frame::OPT: {
                arr.AddItemToArray(cJSON_CreateString(f->opt().c_str()));
            } break;
            default: unreachable();
            }
        }
        *s += arr.PrintUnformatted();
    }

    if (r->has_profile()) {
        *s += ",\"p\":";
        const Datum *d = &r->profile();
        guarantee(d->type() == Datum::R_JSON);
        *s += d->r_str();
    }

    *s += "}";
    return r->token();
}


} // namespace json_shim
