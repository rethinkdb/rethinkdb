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
    rassert(lhs->type == cJSON_Object);
    rassert(rhs->type == cJSON_Object);
    cJSON *obj = cJSON_DeepCopy(lhs);

    for (int i = 0; i < cJSON_GetArraySize(rhs); ++i) {
        cJSON *item = cJSON_GetArrayItem(rhs, i);
        cJSON_DeleteItemFromObject(obj, item->string);
        cJSON_AddItemToObject(obj, item->string, cJSON_DeepCopy(item));
    }
    return obj;
}

std::string cJSON_Print_lexicographic(const cJSON *json) {
    std::string acc;
    rassert(json->type == cJSON_Number || json->type == cJSON_String);
    if (json->type == cJSON_Number) {
        acc += "N";

        union {double d; int64_t u;} packed;
        rassert(sizeof(packed.d) == sizeof(packed.u));
        rassert((void *)&packed.d == (void *)&packed.u);
        packed.d = json->valuedouble;
        acc += strprintf("%.*lx", (int)(sizeof(double)*2), packed.u);
        return acc;
    } else {
        rassert(json->type == cJSON_String);
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

std::string cJSON_Print_std(cJSON *json) {
    char *s = cJSON_Print(json);
    rassert(s);
    std::string res(s);
    free(s);
    return res;
}

/* Render a cJSON entity to text for transfer/storage. */
std::string scoped_cJSON_t::Print() const THROWS_NOTHING {
    return cJSON_Print_std(val);
}
/* Render a cJSON entity to text for transfer/storage without any formatting. */
std::string scoped_cJSON_t::PrintUnformatted() const THROWS_NOTHING {
    char *s = cJSON_PrintUnformatted(val);
    rassert(s);
    std::string res(s);
    free(s);

    return res;
}

std::string scoped_cJSON_t::PrintLexicographic() const THROWS_NOTHING {
    return cJSON_Print_lexicographic(val);
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

copyable_cJSON_t::copyable_cJSON_t(cJSON *_val)
    : val(_val)
{ }

copyable_cJSON_t::copyable_cJSON_t(const copyable_cJSON_t &other)
    : val(cJSON_DeepCopy(other.val))
{ }

copyable_cJSON_t::~copyable_cJSON_t() {
    cJSON_Delete(val);
}

cJSON *copyable_cJSON_t::get() const {
    return val;
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
    rassert(target);
    rassert(target->type == cJSON_Object);
}

json_array_iterator_t::json_array_iterator_t(cJSON *target)
    : json_iterator_t(target)
{
    rassert(target);
    rassert(target->type == cJSON_Array);
}

std::string cJSON_print_std_string(cJSON *json) THROWS_NOTHING {
    rassert(json);
    char *s = cJSON_Print(json);
    rassert(s);
    std::string res(s);
    free(s);

    return res;
}

std::string cJSON_print_unformatted_std_string(cJSON *json) THROWS_NOTHING {
    rassert(json);
    char *s = cJSON_PrintUnformatted(json);
    rassert(s);
    std::string res(s);
    free(s);

    return res;
}

void project(cJSON *json, std::set<std::string> keys) {
    rassert(json);
    rassert(json->type == cJSON_Object);

    json_object_iterator_t it(json);

    std::vector<std::string> keys_to_delete;

    while (cJSON *node = it.next()) {
        rassert(node->string);
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
        rassert(hd->string);
        keys.insert(hd->string);
    }

    for (std::set<std::string>::iterator it = keys.begin();
                                         it != keys.end();
                                         ++it) {
        cJSON_AddItemToObject(res, it->c_str(), cJSON_DetachItemFromObject(x, it->c_str()));
    }

    keys.clear();

    while ((hd = yit.next())) {
        rassert(hd->string);
        keys.insert(hd->string);
    }

    for (std::set<std::string>::iterator it = keys.begin();
                                         it != keys.end();
                                         ++it) {
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
    case cJSON_String:
        {
            rassert(cjson.valuestring);
            std::string s(cjson.valuestring);
            msg << s;
        }
        break;
    case cJSON_Array:
    case cJSON_Object:
        {
            msg << cJSON_GetArraySize(&cjson);

            cJSON *hd = cjson.child;
            while (hd) {
                if (cjson.type == cJSON_Object) {
                    rassert(hd->string);
                    msg << std::string(hd->string);
                }
                msg << *hd;
                hd = hd->next;
            }
        }
        break;
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
