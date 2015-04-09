#include "protob/json_shim.hpp"

#include <inttypes.h>

#include "debug.hpp"
#include "http/json.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "utils.hpp"

std::map<std::string, int32_t> resolver;

namespace json_shim {

class exc_t : public std::exception {
public:
    exc_t() { }
    ~exc_t() throw () { }
    const char *what() const throw () { return "json_shim::exc_t"; }
};

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
void transfer(cJSON *json, T *dest, void (T::*setter)(U)) {
    U tmp;
    if (json != NULL) {
        safe_extract(json, &tmp);
        (dest->*setter)(std::move(tmp));
    }
}

template<class T, class U>
void transfer(cJSON *json, T *dest, U *(T::*mut)()) {
    if (json != NULL) {
        safe_extract(json, (dest->*mut)());
    }
}

template<class T, class U>
void transfer_arr(cJSON *arr, T *dest, U *(T::*adder)()) {
    if (arr != NULL) {
        if (arr->type != cJSON_Object && arr->type != cJSON_Array) throw exc_t();
        for (cJSON *item = arr->head; item != NULL; item = item->next) {
            safe_extract(item, (dest->*adder)());
        }
    }
}

template<>
void extract(cJSON *field, std::string *s) {
    if (field->type != cJSON_String) throw exc_t();
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
    if (json->type == cJSON_Array) {
        // It's ok to use the slow functions here, because the indexes are small
        transfer(cJSON_slow_GetArrayItem(json, 0), t, &Term::set_type);
        transfer_arr(cJSON_slow_GetArrayItem(json, 1), t, &Term::add_args);
        transfer_arr(cJSON_slow_GetArrayItem(json, 2), t, &Term::add_optargs);
    } else if (json->type == cJSON_Object) {
        t->set_type(Term::MAKE_OBJ);
        transfer_arr(json, t, &Term::add_optargs);
    } else {
        t->set_type(Term::DATUM);
        transfer(json, t, &Term::mutable_datum);
    }
}

template<>
void extract(cJSON *json, Datum *d) {
    switch (json->type) {
    case cJSON_False:
        d->set_type(Datum::R_BOOL);
        d->set_r_bool(false);
        break;
    case cJSON_True:
        d->set_type(Datum::R_BOOL);
        d->set_r_bool(true);
        break;
    case cJSON_NULL:
        d->set_type(Datum::R_NULL);
        break;
    case cJSON_Number:
        d->set_type(Datum::R_NUM);
        d->set_r_num(json->valuedouble);
        break;
    case cJSON_String:
        d->set_type(Datum::R_STR);
        d->set_r_str(json->valuestring);
        break;
    case cJSON_Array:
        {
            d->set_type(Datum::R_ARRAY);
            for (cJSON *item = json->head; item != NULL; item = item->next) {
                extract(item, d->add_r_array());
            }
        }
        break;
    case cJSON_Object:
        {
            d->set_type(Datum::R_OBJECT);
            for (cJSON *item = json->head; item != NULL; item = item->next) {
                Datum::AssocPair *ap = d->add_r_object();
                ap->set_key(item->string);
                extract(item, ap->mutable_val());
            }
        }
        break;
    default:
        unreachable();
    }
}

template<>
void extract(cJSON *json, Query::AssocPair *ap) {
    if (json->string == NULL) throw exc_t();
    ap->set_key(json->string);
    extract(json, ap->mutable_val());
}

template<>
void extract(cJSON *json, Term::AssocPair *ap) {
    if (json->string == NULL) throw exc_t();
    ap->set_key(json->string);
    extract(json, ap->mutable_val());
}

template<>
void extract(cJSON *json, Datum::AssocPair *ap) {
    if (json->string == NULL) throw exc_t();
    ap->set_key(json->string);
    extract(json, ap->mutable_val());
}

template<>
void extract(cJSON *json, Query *q) {
    // It's ok to use the slow functions here, because the indexes are small
    transfer(cJSON_slow_GetArrayItem(json, 0), q, &Query::set_type);
    transfer(cJSON_slow_GetArrayItem(json, 1), q, &Query::mutable_query);
    q->set_accepts_r_json(true);
    transfer_arr(cJSON_slow_GetArrayItem(json, 2), q, &Query::add_global_optargs);
}

bool parse_json_pb(Query *q, int64_t token, const char *str) THROWS_NOTHING {
    try {
        q->Clear();
        q->set_token(token);
        scoped_cJSON_t json_holder(cJSON_Parse(str));
        cJSON *json = json_holder.get();
        if (json == NULL) return false;
        extract(json, q);
        return true;
    } catch (const exc_t &) {
        // This happens if the user provides bad JSON.  TODO: Give the user a
        // more specific error than "malformed query".
        return false;
    } catch (const google::protobuf::FatalException &) {
        return false;
    } catch (...) {
        // If we get an unexpected error, we only rethrow in debug mode.  (This
        // is consistent with the general principle that queries shouldn't crash
        // the server even if the ReQL logic is incorrect; see also
        // `r_sanity_check`.)
#ifndef NDEBUG
        throw;
#else
        return false;
#endif // NDEBUG
    }
}

void write_json_pb(const Response &r, std::string *s) THROWS_NOTHING {
    // Note: We must keep any existing prefix in `s` intact.
#ifdef NDEBUG
    const size_t start_offset = s->length();
#endif
    try {
        *s += strprintf("{\"t\":%d,\"r\":[", r.type());
        for (int i = 0; i < r.response_size(); ++i) {
            *s += (i == 0) ? "" : ",";
            const Datum *d = &r.response(i);
            if (d->type() == Datum::R_JSON) {
                *s += d->r_str();
            } else if (d->type() == Datum::R_STR) {
                scoped_cJSON_t tmp(cJSON_CreateString(d->r_str().c_str()));
                *s += tmp.PrintUnformatted();
            } else {
                unreachable();
            }
        }
        *s += "],\"n\":[";
        for (int i = 0; i < r.notes_size(); ++i) {
            *s += (i == 0) ? "" : ",";
            *s += strprintf("%d", r.notes(i));
        }
        *s += "]";

        if (r.has_backtrace()) {
            *s += ",\"b\":";
            const Backtrace *bt = &r.backtrace();
            scoped_cJSON_t arr(cJSON_CreateArray());
            for (int i = 0; i < bt->frames_size(); ++i) {
                const Frame *f = &bt->frames(i);
                switch (f->type()) {
                case Frame::POS:
                    arr.AddItemToArray(cJSON_CreateNumber(f->pos()));
                    break;
                case Frame::OPT:
                    arr.AddItemToArray(cJSON_CreateString(f->opt().c_str()));
                    break;
                default:
                    unreachable();
                }
            }
            *s += arr.PrintUnformatted();
        }

        if (r.has_profile()) {
            *s += ",\"p\":";
            const Datum *d = &r.profile();
            guarantee(d->type() == Datum::R_JSON);
            *s += d->r_str();
        }

        *s += "}";
    } catch (...) {
#ifndef NDEBUG
        throw;
#else
        // Erase everything we have added above, then append an error message
        s->erase(start_offset);
        *s += strprintf("{\"t\":%d,\"r\":[\"%s\"]}",
                        Response::RUNTIME_ERROR,
                        "Internal error in `write_json_pb`, please report this.");
#endif // NDEBUG
    }
}


} // namespace json_shim
