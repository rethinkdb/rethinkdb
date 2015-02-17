#include "protob/json_shim.hpp"

#include <inttypes.h>

#include "debug.hpp"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rdb_protocol/ql2.pb.h"
#include "utils.hpp"

std::map<std::string, int32_t> resolver;

using namespace rapidjson;

namespace json_shim {

class exc_t : public std::exception {
public:
    exc_t() { }
    ~exc_t() throw () { }
    const char *what() const throw () { return "json_shim::exc_t"; }
};

// The first value is != nullptr if this is a key/value pair from an object.
// In that case it is the key and the second value the value.
template<class T>
typename std::enable_if<!((std::is_enum<T>::value || std::is_fundamental<T>::value)
                          && !std::is_same<T, bool>::value)>::type
extract(const Value *, const Value &, T *);

template<class T>
typename std::enable_if<(std::is_enum<T>::value || std::is_fundamental<T>::value)
                        && !std::is_same<T, bool>::value>::type
extract(const Value *, const Value &field, T *dest) {
    if (!field.IsNumber()) {
        throw exc_t();
    }
    T t = static_cast<T>(field.GetDouble());
    if (static_cast<double>(t) != field.GetDouble()) {
        throw exc_t();
    }
    *dest = t;
}

template<class T>
void safe_extract(const Value *key, const Value &val, T *t) {
    if (t != nullptr) {
        extract(key, val, t);
    }
}

template<class T, class U>
void transfer(const Value &json, T *dest, void (T::*setter)(U)) {
    U tmp;
    safe_extract(nullptr, json, &tmp);
    (dest->*setter)(std::move(tmp));
}

template<class T, class U>
void transfer(const Value &json, T *dest, U *(T::*mut)()) {
    safe_extract(nullptr, json, (dest->*mut)());
}

template<class T, class U>
void transfer_arr(const Value &arr, T *dest, U *(T::*adder)()) {
    if (arr.IsArray()) {
        for (Value::ConstValueIterator it = arr.Begin(); it != arr.End(); ++it) {
            safe_extract(nullptr, *it, (dest->*adder)());
        }
    } else if (arr.IsObject()) {
        for (Value::ConstMemberIterator it = arr.MemberBegin();
             it != arr.MemberEnd();
             ++it) {
            safe_extract(&it->name, it->value, (dest->*adder)());
        }
    } else {
        throw exc_t();
    }
}

template<>
void extract(const Value *, const Value &field, std::string *s) {
    if (!field.IsString()) {
        throw exc_t();
    } else {
        *s = field.GetString();
    }
}

template<>
void extract(const Value *, const Value &field, bool *dest) {
    if (!field.IsBool()) {
        throw exc_t();
    } else {
        *dest = field.GetBool();
    }
}

template<>
void extract(const Value *, const Value &json, Term *t) {
    if (json.IsArray()) {
        if (json.Size() > 0) {
            transfer(json[0], t, &Term::set_type);
        }
        if (json.Size() > 1) {
            transfer_arr(json[1], t, &Term::add_args);
        }
        if (json.Size() > 2) {
            transfer_arr(json[2], t, &Term::add_optargs);
        }
    } else if (json.IsObject()) {
        t->set_type(Term::MAKE_OBJ);
        transfer_arr(json, t, &Term::add_optargs);
    } else {
        t->set_type(Term::DATUM);
        transfer(json, t, &Term::mutable_datum);
    }
}

template<>
void extract(const Value *, const Value &json, Datum *d) {
    // TODO (daniel): Maybe use an enum here again, using json.GetType()?
    if (json.IsBool()) {
        d->set_type(Datum::R_BOOL);
        d->set_r_bool(json.GetBool());
    } else if (json.IsNull()) {
        d->set_type(Datum::R_NULL);
    } else if (json.IsNumber()) {
        d->set_type(Datum::R_NUM);
        d->set_r_num(json.GetDouble());
    } else if (json.IsString()) {
        d->set_type(Datum::R_STR);
        d->set_r_str(json.GetString());
    } else if (json.IsArray()) {
        d->set_type(Datum::R_ARRAY);
        for (Value::ConstValueIterator it = json.Begin(); it != json.End(); ++it) {
            extract(nullptr, *it, d->add_r_array());
        }
    } else if (json.IsObject()) {
        d->set_type(Datum::R_OBJECT);
        for (Value::ConstMemberIterator it = json.MemberBegin();
             it != json.MemberEnd();
             ++it) {
            Datum::AssocPair *ap = d->add_r_object();
            ap->set_key(it->name.GetString());
            extract(nullptr, it->value, ap->mutable_val());
        }
    } else {
        unreachable();
    }
}

template<>
void extract(const Value *key, const Value &val, Query::AssocPair *ap) {
    if (key == nullptr || !key->IsString()) throw exc_t();
    ap->set_key(key->GetString());
    extract(nullptr, val, ap->mutable_val());
}

template<>
void extract(const Value *key, const Value &val, Term::AssocPair *ap) {
    if (key == nullptr || !key->IsString()) throw exc_t();
    ap->set_key(key->GetString());
    extract(nullptr, val, ap->mutable_val());
}

template<>
void extract(const Value *key, const Value &val, Datum::AssocPair *ap) {
    if (key == nullptr || !key->IsString()) throw exc_t();
    ap->set_key(key->GetString());
    extract(nullptr, val, ap->mutable_val());
}

template<>
void extract(const Value *, const Value &json, Query *q) {
    if (json.Size() > 0) {
        transfer(json[0], q, &Query::set_type);
    }
    if (json.Size() > 1) {
        transfer(json[1], q, &Query::mutable_query);
    }
    q->set_accepts_r_json(true);
    if (json.Size() > 2) {
        transfer_arr(json[2], q, &Query::add_global_optargs);
    }
}

bool parse_json_pb(Query *q, int64_t token, const char *str) THROWS_NOTHING {
    try {
        q->Clear();
        q->set_token(token);
        Document json;
        // TODO! Use Insitu parsing
        json.Parse(str);
        // TODO: We should return not just `false`, but the error message
        if (json.HasParseError()) {
            return false;
        }
        extract(nullptr, json, q);
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
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    try {
        writer.StartObject();
        writer.Key("t", 1);
        writer.Uint(r.type());
        writer.Key("r", 1);
        writer.StartArray();
        for (int i = 0; i < r.response_size(); ++i) {
            const Datum *d = &r.response(i);
            if (d->type() == Datum::R_JSON) {
                // Just put the raw JSON onto the buffer
                // TODO! Check length
                char *out =buffer.Push(d->r_str().length());
                memcpy(out, d->r_str().data(), d->r_str().length());
            } else if (d->type() == Datum::R_STR) {
                writer.String(d->r_str().data(), d->r_str().length());
            } else {
                unreachable();
            }
        }
        writer.EndArray();

        // TODO! Implement
        /*if (r.has_backtrace()) {
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
        }*/

        writer.EndObject();
    } catch (...) {
#ifndef NDEBUG
        throw;
#else
        writer.StartObject();
        writer.Key("t", 1);
        writer.Uint(Response::RUNTIME_ERROR);
        writer.Key("r", 1);
        writer.StartArray();
        writer.String("Internal error in `write_json_pb`, please report this.");
        writer.EndArray();
        writer.EndObject();
#endif // NDEBUG
    }

    s->append(buffer.GetString(), buffer.GetSize());
}


} // namespace json_shim
