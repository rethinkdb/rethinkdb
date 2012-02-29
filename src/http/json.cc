#include "http/json.hpp"
#include <stdlib.h>

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
    delete s;

    return res;
}
