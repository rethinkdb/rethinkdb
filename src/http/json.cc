#include "http/json.hpp"

scoped_cJSON_t::scoped_cJSON_t(cJSON *_val) 
    : val(_val)
{ }

scoped_cJSON_t::~scoped_cJSON_t() {
    cJSON_Delete(val);
}

cJSON *scoped_cJSON_t::get() {
    return val;
}
