// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "cjson/json.hpp"

#include <inttypes.h>
#include <stdlib.h>

#include <set>
#include <vector>

#include "stl_utils.hpp"
#include "utils.hpp"

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
        if (packed.u & (1ULL << 63)) {
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
            packed.u ^= (1ULL << 63);
        }

        acc += strprintf("%.*" PRIx64, static_cast<int>(sizeof(double) * 2), packed.u);
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
    node = target->head;
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
