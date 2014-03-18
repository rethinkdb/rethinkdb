#include "protob/json_shim.hpp"

#include "http/json.hpp"
#include "rdb_protocol/ql2.pb.h"

std::map<std::string, int32_t> resolver;

namespace json_shim {

class exc_t : public std::exception {
public:
    exc_t(cJSON *) : str("PLACEHOLDER") { }
    const char *what() const throw () { return str.c_str(); }
private:
    std::string str;
};

void check_type(cJSON *json, int expected) {
    if (json->type != expected) throw exc_t(json);
}

// RSI: parenthesize
#define TRANSFER(JSON, FIELD, DEST) do {                                \
        struct {                                                        \
            template<class T__, class = void>                           \
            struct t__;                                                 \
            template<class T__>                                         \
            struct t__<T__, typename                                    \
                       std::enable_if<&T__::set_##FIELD != NULL>::type> { \
                void operator()(cJSON *json__, T__ *dest__) {           \
                    decltype(dest__->FIELD()) tmp__;                    \
                    extractor_t<decltype(tmp__)>()(                     \
                        cJSON_GetObjectItem(json__, #FIELD), &tmp__);   \
                    dest__->set_##FIELD(std::move(tmp__));              \
                }                                                       \
            };                                                          \
            template<class T__>                                         \
            struct t__<T__, typename                                    \
                       std::enable_if<&T__::release_##FIELD != NULL>::type> { \
                void operator()(cJSON *json__, T__ *dest__) {           \
                    extractor_t<decltype(dest__->FIELD())>()(           \
                        cJSON_GetObjectItem(json__, #FIELD),            \
                        dest__->mutable_##FIELD());                     \
                }                                                       \
            };                                                          \
            template<class T__>                                         \
            struct t__<T__, typename                                    \
                       std::enable_if<&T__::FIELD##_size != NULL>::type> { \
                void operator()(cJSON *json__, T__ *dest__) {           \
                    cJSON *arr__ = cJSON_GetObjectItem(json__, #FIELD); \
                    check_type(arr__, cJSON_Array);                     \
                    int64_t sz__ = cJSON_GetArraySize(arr__);           \
                    for (int64_t i__ = 0; i__ < sz__; ++i__) {          \
                        auto el__ = dest__->add_##FIELD();              \
                        extractor_t<decltype(dest__->FIELD(0))>()(      \
                            cJSON_GetArrayItem(arr__, i__),             \
                            el__);                                      \
                    }                                                   \
                }                                                       \
            };                                                          \
            template<class T>                                           \
            void operator()(cJSON *json__, T *dest__) {                 \
                t__<T>()(json__, dest__);                               \
            }                                                           \
        } meta_extractor__;                                             \
        check_type(JSON, cJSON_Object);                                 \
        meta_extractor__(JSON, DEST);                                   \
    } while (0)

template<class T, class = void>
struct extractor_t;

template<class T>
struct extractor_t<T, typename std::enable_if<std::is_enum<T>::value
                                              || std::is_integral<T>::value>::type> {
    void operator()(cJSON *field, T *dest) {
        if (field->type != cJSON_Number) throw exc_t(field);
        T t = static_cast<T>(field->valuedouble);
        if (static_cast<double>(t) != field->valuedouble) throw exc_t(field);
        *dest = t;
    }
};

template<>
struct extractor_t<const Query::AssocPair &> {
    void operator()(cJSON *field, Query::AssocPair *ap) {
        TRANSFER(field, key, ap);
        // TRANSFER(field, val, ap);
    };
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

bool parse_json_pb(Query *q, char *str) {
    q->Clear();
    scoped_cJSON_t json_holder(cJSON_Parse(str));
    cJSON *json = json_holder.get();
    if (json == NULL) return false;
    TRANSFER(json, type, q);
    // query
    TRANSFER(json, token, q);
    TRANSFER(json, global_optargs, q);

    return true;
}

} // namespace json_shim
