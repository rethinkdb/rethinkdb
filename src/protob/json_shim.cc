#include <inttypes.h>
#include "protob/json_shim.hpp"

#include "utils.hpp"

#include "http/json.hpp"
#include "rdb_protocol/ql2.pb.h"

std::map<std::string, int32_t> resolver;

namespace json_shim {

class exc_t : public std::exception {
public:
    exc_t(cJSON *) : str("PLACEHOLDER") { BREAKPOINT; }
    const char *what() const throw () { return str.c_str(); }
private:
    std::string str;
};

void check_type(cJSON *json, int expected) {
    if (json->type != expected) throw exc_t(json);
}

// RSI: parenthesize
#define TRANSFER(JSON, FIELD, NAME, DEST) do {                          \
struct {                                                                \
    template<class T__, class = void>                                   \
    struct t__;                                                         \
    template<class T__>                                                 \
    struct t__<T__, typename                                            \
               std::enable_if<&T__::set_##FIELD != NULL>::type> {       \
        void operator()(cJSON *json__, T__ *dest__) {                   \
            decltype(dest__->FIELD()) tmp__;                            \
            cJSON *item__ = cJSON_GetObjectItem(json__, NAME);          \
            if (item__ == NULL) return;                                 \
            safe_extractor_t<decltype(tmp__)>()(item__, &tmp__);        \
            dest__->set_##FIELD(std::move(tmp__));                      \
        }                                                               \
    };                                                                  \
    template<class T__>                                                 \
    struct t__<T__, typename                                            \
               std::enable_if<&T__::release_##FIELD != NULL>::type> {   \
        void operator()(cJSON *json__, T__ *dest__) {                   \
            cJSON *item__ = cJSON_GetObjectItem(json__, NAME);          \
            if (item__ == NULL) return;                                 \
            safe_extractor_t<decltype(dest__->FIELD())>()(              \
                item__, dest__->mutable_##FIELD());                     \
        }                                                               \
    };                                                                  \
    template<class T__>                                                 \
    struct t__<T__, typename                                            \
               std::enable_if<&T__::FIELD##_size != NULL>::type> {      \
        void operator()(cJSON *json__, T__ *dest__) {                   \
            cJSON *arr__ = cJSON_GetObjectItem(json__, NAME);           \
            if (arr__ == NULL) return;                                  \
            check_type(arr__, cJSON_Array);                             \
            int64_t sz__ = cJSON_GetArraySize(arr__);                   \
            for (int64_t i__ = 0; i__ < sz__; ++i__) {                  \
                auto el__ = dest__->add_##FIELD();                      \
                safe_extractor_t<decltype(dest__->FIELD(0))>()(         \
                    cJSON_GetArrayItem(arr__, i__),                     \
                    el__);                                              \
            }                                                           \
        }                                                               \
    };                                                                  \
    template<class T>                                                   \
    void operator()(cJSON *json__, T *dest__) {                         \
        t__<T>()(json__, dest__);                                       \
    }                                                                   \
} meta_extractor__;                                                     \
check_type(JSON, cJSON_Object);                                         \
meta_extractor__(JSON, DEST);                                           \
} while (0)

template<class T, class = void>
struct extractor_t;

template<class T>
struct safe_extractor_t : public extractor_t<T> {
    template<class U>
    void operator()(cJSON *json, U *t) {
        if (json != NULL && t != NULL) {
            (*static_cast<extractor_t<T> *>(this))(json, t);
        }
    }
};

template<class T>
struct extractor_t<
    T, typename std::enable_if<std::is_enum<T>::value
                               || std::is_fundamental<T>::value>::type> {
    void operator()(cJSON *field, T *dest) {
        if (field->type != cJSON_Number) throw exc_t(field);
        T t = static_cast<T>(field->valuedouble);
        if (static_cast<double>(t) != field->valuedouble) throw exc_t(field);
        *dest = t;
    }
};

template<>
struct extractor_t<const std::string &> {
    void operator()(cJSON *field, std::string *s) {
        check_type(field, cJSON_String);
        *s = field->valuestring;
    };
};

template<>
struct extractor_t<bool> {
    void operator()(cJSON *field, bool *dest) {
        if (field->type == cJSON_False) {
            *dest = false;
        } else if (field->type == cJSON_True) {
            *dest = true;
        } else {
            throw exc_t(field);
        }
    }
};

template<>
struct extractor_t<const Query::AssocPair &> {
    void operator()(cJSON *json, Query::AssocPair *ap) {
        TRANSFER(json, key, "k", ap);
        TRANSFER(json, val, "v", ap);
    };
};

template<>
struct extractor_t<const Term::AssocPair &> {
    void operator()(cJSON *json, Term::AssocPair *ap) {
        TRANSFER(json, key, "k", ap);
        TRANSFER(json, val, "v", ap);
    };
};

template<>
struct extractor_t<const Datum::AssocPair &> {
    void operator()(cJSON *json, Datum::AssocPair *ap) {
        TRANSFER(json, key, "k", ap);
        TRANSFER(json, val, "v", ap);
    };
};

template<>
struct extractor_t<const Query &> {
    void operator()(cJSON *json, Query *q) {
        TRANSFER(json, type, "t", q);
        TRANSFER(json, query, "q", q);
        TRANSFER(json, token, "k", q);
        q->set_accepts_r_json(true); // RSI: response should be true JSON
        TRANSFER(json, global_optargs, "g", q);
    };
};

template<>
struct extractor_t<const Term &> {
    void operator()(cJSON *json, Term *t) {
        TRANSFER(json, type, "t", t);
        TRANSFER(json, datum, "d", t);
        TRANSFER(json, args, "a", t);
        TRANSFER(json, optargs, "o", t);
    }
};

template<>
struct extractor_t<const Datum &> {
    void operator()(cJSON *json, Datum *d) {
        TRANSFER(json, type, "t", d);
        switch (d->type()) {
        case Datum::R_NULL:                                     break;
        case Datum::R_BOOL:   TRANSFER(json, r_bool,   "b", d); break;
        case Datum::R_NUM:    TRANSFER(json, r_num,    "n", d); break;
        case Datum::R_STR:    TRANSFER(json, r_str,    "s", d); break;
        case Datum::R_ARRAY:  TRANSFER(json, r_array,  "a", d); break;
        case Datum::R_OBJECT: TRANSFER(json, r_object, "o", d); break;
        case Datum::R_JSON:   TRANSFER(json, r_str,    "s", d); break;
        default: unreachable();
        }
    }
};

bool parse_json_pb(Query *q, char *str) throw () {
    // debug_timer_t tm;
    q->Clear();
    scoped_cJSON_t json_holder(cJSON_Parse(str));
    // tm.tick("JSON");
    cJSON *json = json_holder.get();
    if (json == NULL) return false;
    extractor_t<const Query &>()(json, q);
    // tm.tick("SHIM");
    return true;
}

std::string write_json_pb(const Response *r) throw () {
    std::string s;
    s += strprintf("{\"t\":%d,\"k\":%" PRIi64 ",\"r\":[", r->type(), r->token());
    for (int i = 0; i < r->response_size(); ++i) {
        s += (i == 0) ? "" : ",";
        const Datum *d = &r->response(i);
        if (d->type() == Datum::R_JSON) {
            s += d->r_str();
        } else if (d->type() == Datum::R_STR) {
            scoped_cJSON_t tmp(cJSON_CreateString(d->r_str().c_str()));
            s += tmp.PrintUnformatted();
        } else {
            unreachable();
        }
    }
    s += "]";

    if (r->has_backtrace()) {
        s += ",\"b\":";
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
        s += arr.PrintUnformatted();
    }

    if (r->has_profile()) {
        s += ",\"p\":";
        const Datum *d = &r->profile();
        guarantee(d->type() == Datum::R_JSON);
        s += d->r_str();
    }

    s += "}";

    return std::move(s);
}


} // namespace json_shim
