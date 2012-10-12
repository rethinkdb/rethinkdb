#include "containers/name_string.hpp"

#include <ctype.h>

bool is_acceptable_name_character(int ch) {
    return isalpha(ch) || isdigit(ch) || ch == '_';
}

name_string_t::name_string_t() { }

bool name_string_t::assign(const std::string& s) {
    for (size_t i = 0; i < s.size(); ++i) {
        if (!is_acceptable_name_character(s[i])) {
            return false;
        }
    }
    str_ = s;
    return true;
}
