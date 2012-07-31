#include <stdlib.h>

#include <set>
#include <vector>

#include "http/json.hpp"
#include "stl_utils.hpp"
#include "utils.hpp"

#ifndef NDEBUG
std::string (*cJSON_default_print)(cJSON *json) = cJSON_print_std_string;
#else
std::string (*cJSON_default_print)(cJSON *json) = cJSON_print_unformatted_std_string;
#endif
http_res_t http_json_res(cJSON *json) {
    return http_res_t(HTTP_OK, "application/json", cJSON_default_print(json));
}

scoped_cJSON_t::scoped_cJSON_t(cJSON *_val)
    : val(_val)
{ }

scoped_cJSON_t::~scoped_cJSON_t() {
    if (val) {
        cJSON_Delete(val);
    }
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
    rassert(target->type == cJSON_Object);
}

json_array_iterator_t::json_array_iterator_t(cJSON *target)
    : json_iterator_t(target)
{
    rassert(target->type == cJSON_Array);
}

std::string cJSON_print_std_string(cJSON *json) {
    char *s = cJSON_Print(json);
    std::string res(s);
    free(s);

    return res;
}

std::string cJSON_print_unformatted_std_string(cJSON *json) {
    char *s = cJSON_PrintUnformatted(json);
    std::string res(s);
    free(s);

    return res;
}

void project(cJSON *json, std::set<std::string> keys) {
    rassert(json->type == cJSON_Object);

    json_object_iterator_t it(json);

    std::vector<std::string> keys_to_delete;

    while (cJSON *node = it.next()) {
        std::string str(node->string);
        if (!std_contains(keys, str)) {
            keys_to_delete.push_back(str);
        }
    }

    for (std::vector<std::string>::iterator it  = keys_to_delete.begin();
                                            it != keys_to_delete.end();
                                            ++it) {
        cJSON_DeleteItemFromObject(json, it->c_str());
    }
}

cJSON *merge(cJSON *x, cJSON *y) {
    cJSON *res = cJSON_CreateObject();
    json_object_iterator_t xit(x), yit(y);

    std::set<std::string> keys;
    cJSON *hd;

    while ((hd = xit.next())) {
        keys.insert(hd->string);
    }

    for (std::set<std::string>::iterator it = keys.begin();
                                         it != keys.end();
                                         ++it) {
        cJSON_AddItemToObject(res, it->c_str(), cJSON_DetachItemFromObject(x, it->c_str()));
    }

    keys.clear();

    while ((hd = yit.next())) {
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
