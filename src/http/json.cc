#include "http/json.hpp"

#include <stdlib.h>

#include <set>
#include <vector>

#include "http/http.hpp"
#include "stl_utils.hpp"
#include "utils.hpp"
#include "containers/archive/stl_types.hpp"

#ifndef NDEBUG
std::string (*cJSON_default_print)(cJSON *json) = cJSON_print_std_string;
#else
std::string (*cJSON_default_print)(cJSON *json) = cJSON_print_unformatted_std_string;
#endif
http_res_t http_json_res(cJSON *json) {
    return http_res_t(HTTP_OK, "application/json", cJSON_default_print(json));
}

cJSON *cJSON_merge(cJSON *lhs, cJSON *rhs) {
    guarantee(lhs->type == cJSON_Object);
    guarantee(rhs->type == cJSON_Object);
    cJSON *obj = cJSON_DeepCopy(lhs);

    for (int i = 0; i < cJSON_GetArraySize(rhs); ++i) {
        cJSON *item = cJSON_GetArrayItem(rhs, i);
        cJSON_DeleteItemFromObject(obj, item->string);
        cJSON_AddItemToObject(obj, item->string, cJSON_DeepCopy(item));
    }
    return obj;
}

std::string cJSON_print_lexicographic(const cJSON *json) {
    std::string acc;
    guarantee(json->type == cJSON_Number || json->type == cJSON_String);
    if (json->type == cJSON_Number) {
        acc += "N";

        union {
            double d;
            int64_t u;
        } packed;
        rassert(sizeof(packed.d) == sizeof(packed.u));
        packed.d = json->valuedouble;

        // Mangle the value so that lexicographic ordering matches double ordering
        if (packed.u & (1UL << 63)) {
            // If we have a negative double, flip all the bits.  Flipping the
            // highest bit causes the negative doubles to sort below the
            // positive doubles (which will also have their highest bit
            // flipped), and flipping all the other bits causes more negative
            // doubles to sort below less negative doubles.
            packed.u = ~packed.u;
        } else {
            // If we have a non-negative double, flip the highest bit so that it
            // sorts higher than all the negative doubles (which had their
            // highest bit flipped as well).
            packed.u ^= (1UL << 63);
        }

        acc += strprintf("%.*lx", static_cast<int>(sizeof(double)*2), packed.u);
        acc += strprintf("#%.20g", json->valuedouble);
    } else {
        guarantee(json->type == cJSON_String);
        acc += "S";
        acc += json->valuestring;
    }
    return acc;
}

scoped_cJSON_t::scoped_cJSON_t(cJSON *_val)
    : val(_val)
{ }

scoped_cJSON_t::~scoped_cJSON_t() {
    if (val) {
        cJSON_Delete(val);
    }
}

/* Render a cJSON entity to text for transfer/storage. */
std::string scoped_cJSON_t::Print() const THROWS_NOTHING {
    return cJSON_print_std_string(val);
}
/* Render a cJSON entity to text for transfer/storage without any formatting. */
std::string scoped_cJSON_t::PrintUnformatted() const THROWS_NOTHING {
    char *s = cJSON_PrintUnformatted(val);
    guarantee(s);
    std::string res(s);
    free(s);

    return res;
}

std::string scoped_cJSON_t::PrintLexicographic() const THROWS_NOTHING {
    return cJSON_print_lexicographic(val);
}

cJSON *scoped_cJSON_t::get() const {
    return val;
}

cJSON *scoped_cJSON_t::release() {
    cJSON *tmp = val;
    val = NULL;
    return tmp;
}

void scoped_cJSON_t::reset(cJSON *v) {
    if (val) {
        cJSON_Delete(val);
    }
    val = v;
}

json_iterator_t::json_iterator_t(cJSON *target) {
    node = target->child;
}

cJSON *json_iterator_t::next() {
    cJSON *res = node;
    if (node) {
        node = node->next;
    }
    return res;
}

json_object_iterator_t::json_object_iterator_t(cJSON *target)
    : json_iterator_t(target)
{
    guarantee(target);
    guarantee(target->type == cJSON_Object);
}

json_array_iterator_t::json_array_iterator_t(cJSON *target)
    : json_iterator_t(target)
{
    guarantee(target);
    guarantee(target->type == cJSON_Array);
}

std::string cJSON_print_std_string(cJSON *json) THROWS_NOTHING {
    guarantee(json);
    char *s = cJSON_Print(json);
    guarantee(s);
    std::string res(s);
    free(s);

    return res;
}

std::string cJSON_print_unformatted_std_string(cJSON *json) THROWS_NOTHING {
    guarantee(json);
    char *s = cJSON_PrintUnformatted(json);
    guarantee(s);
    std::string res(s);
    free(s);

    return res;
}

void project(cJSON *json, std::set<std::string> keys) {
    guarantee(json);
    guarantee(json->type == cJSON_Object);

    json_object_iterator_t it(json);

    std::vector<std::string> keys_to_delete;

    while (cJSON *node = it.next()) {
        guarantee(node->string);
        std::string str(node->string);
        if (!std_contains(keys, str)) {
            keys_to_delete.push_back(str);
        }
    }

    for (std::vector<std::string>::iterator jt = keys_to_delete.begin(); jt != keys_to_delete.end(); ++jt) {
        cJSON_DeleteItemFromObject(json, jt->c_str());
    }
}

cJSON *merge(cJSON *x, cJSON *y) {
    cJSON *res = cJSON_CreateObject();
    json_object_iterator_t xit(x), yit(y);

    std::set<std::string> keys;
    cJSON *hd;

    while ((hd = xit.next())) {
        guarantee(hd->string);
        keys.insert(hd->string);
    }

    for (std::set<std::string>::iterator it = keys.begin();
                                         it != keys.end();
                                         ++it) {
        cJSON_AddItemToObject(res, it->c_str(), cJSON_DetachItemFromObject(x, it->c_str()));
    }

    keys.clear();

    while ((hd = yit.next())) {
        guarantee(hd->string);
        keys.insert(hd->string);
    }

    for (std::set<std::string>::iterator it = keys.begin();
                                         it != keys.end();
                                         ++it) {
        // TODO: Make this a guarantee, or have a new cJSON_AddItemToObject implementation that checks this.
        rassert(!cJSON_GetObjectItem(res, it->c_str()), "Overlapping names in merge, name was: %s\n", it->c_str());
        cJSON_AddItemToObject(res, it->c_str(), cJSON_DetachItemFromObject(y, it->c_str()));
    }

    return res;
}

std::string cJSON_type_to_string(int type) {
    switch (type) {
    case cJSON_False: return "bool"; break;
    case cJSON_True: return "bool"; break;
    case cJSON_NULL: return "null"; break;
    case cJSON_Number: return "number"; break;
    case cJSON_String: return "string"; break;
    case cJSON_Array: return "array"; break;
    case cJSON_Object: return "object"; break;
    default: unreachable(); break;
    }
}

write_message_t &operator<<(write_message_t &msg, const cJSON &cjson) {
    msg << cjson.type;

    switch(cjson.type) {
    case cJSON_False:
        break;
    case cJSON_True:
        break;
    case cJSON_NULL:
        break;
    case cJSON_Number:
        msg << cjson.valuedouble;
        break;
    case cJSON_String: {
        guarantee(cjson.valuestring);
        std::string s(cjson.valuestring);
        msg << s;
    } break;
    case cJSON_Array:
    case cJSON_Object: {
        msg << cJSON_GetArraySize(&cjson);

        cJSON *hd = cjson.child;
        while (hd) {
            if (cjson.type == cJSON_Object) {
                guarantee(hd->string);
                msg << std::string(hd->string);
            }
            msg << *hd;
            hd = hd->next;
        }
    } break;
    default:
        crash("Unreachable");
        break;
    }
    return msg;
}

MUST_USE archive_result_t deserialize(read_stream_t *s, cJSON *cjson) {
    archive_result_t res = deserialize(s, &cjson->type);
    if (res) { return res; }

    switch (cjson->type) {
    case cJSON_False:
    case cJSON_True:
    case cJSON_NULL:
        return ARCHIVE_SUCCESS;
        break;
    case cJSON_Number:
        res = deserialize(s, &cjson->valuedouble);
        if (res) { return res; }
        cjson->valueint = static_cast<int>(cjson->valuedouble);
        return ARCHIVE_SUCCESS;
        break;
    case cJSON_String:
        {
            std::string str;
            res = deserialize(s, &str);
            if (res) { return res; }
            cjson->valuestring = strdup(str.c_str());
            return ARCHIVE_SUCCESS;
        }
        break;
    case cJSON_Array:
        {
            int size;
            res = deserialize(s, &size);
            if (res) { return res; }
            for (int i = 0; i < size; ++i) {
                cJSON *item = cJSON_CreateBlank();
                res = deserialize(s, item);
                if (res) { return res; }
                cJSON_AddItemToArray(cjson, item);
            }
            return ARCHIVE_SUCCESS;
        }
        break;
    case cJSON_Object:
        {
            int size;
            res = deserialize(s, &size);
            if (res) { return res; }
            for (int i = 0; i < size; ++i) {
                //grab the key
                std::string key;
                res = deserialize(s, &key);
                if (res) { return res; }

                //grab the item
                cJSON *item = cJSON_CreateBlank();
                res = deserialize(s, item);
                if (res) { return res; }
                cJSON_AddItemToObject(cjson, key.c_str(), item);
            }
            return ARCHIVE_SUCCESS;
        }
        break;
    default:
        crash("Unreachable");
        break;
    }
}
