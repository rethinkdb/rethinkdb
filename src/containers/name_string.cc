// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "containers/name_string.hpp"

#include <ctype.h>

#include "containers/archive/stl_types.hpp"
#include "debug.hpp"

const char *const name_string_t::valid_char_msg = "Use A-Za-z0-9_ only";

bool is_acceptable_name_character(int ch) {
    return isalpha(ch) || isdigit(ch) || ch == '_';
}

name_string_t::name_string_t() { }

bool name_string_t::assign_value(const std::string &s) {
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

bool name_string_t::assign_value(const datum_string_t &s) {
    if (s.size() == 0) {
        return false;
    }

    for (size_t i = 0; i < s.size(); ++i) {
        if (!is_acceptable_name_character(s.data()[i])) {
            return false;
        }
    }

    str_ = s.to_std();
    return true;
}

name_string_t name_string_t::guarantee_valid(const char *name) {
    name_string_t string;
    bool ok = string.assign_value(name);
    guarantee(ok, "The name `%s` is not valid.", name);
    return string;
}

RDB_IMPL_ME_SERIALIZABLE_1_SINCE_v1_13(name_string_t, str_);

void debug_print(printf_buffer_t *buf, const name_string_t& s) {
    debug_print(buf, s.str());
}
