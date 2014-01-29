// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "containers/name_string.hpp"

#include <ctype.h>

#include "containers/archive/stl_types.hpp"

const char *const name_string_t::valid_char_msg = "Use A-Za-z0-9_ only";

bool is_acceptable_name_character(int ch) {
    return isalpha(ch) || isdigit(ch) || ch == '_';
}

name_string_t::name_string_t() { }

bool name_string_t::assign_value(const std::string& s) {
    if (s.empty()) {
        return false;
    }

    for (size_t i = 0; i < s.size(); ++i) {
        if (!is_acceptable_name_character(s[i])) {
            return false;
        }
    }

    str_ = s;
    return true;
}

RDB_IMPL_ME_SERIALIZABLE_1(name_string_t, str_);


json_adapter_if_t::json_adapter_map_t get_json_subfields(UNUSED name_string_t *target) {
    std::string fake_target;
    return get_json_subfields(&fake_target);
}

cJSON *render_as_json(name_string_t *target) {
    std::string tmp = target->str();
    return render_as_json(&tmp);
}

void apply_json_to(cJSON *change, name_string_t *target) {
    std::string tmp;
    apply_json_to(change, &tmp);
    if (!target->assign_value(tmp)) {
        throw schema_mismatch_exc_t("invalid name: " + tmp);
    }
}

void debug_print(printf_buffer_t *buf, const name_string_t& s) {
    debug_print(buf, s.str());
}
