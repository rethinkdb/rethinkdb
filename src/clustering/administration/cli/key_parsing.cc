#include "clustering/administration/cli/key_parsing.hpp"

#include "errors.hpp"
#include <boost/algorithm/string.hpp>   /* for `boost::iequals()` */

bool cli_str_to_key(const std::string &str, store_key_t *out) {
    if (boost::iequals(str, "-inf")) {
        *out = store_key_t::min();
        return true;
    }
    size_t p = 0;
    out->set_size(0);
    while (p < str.length()) {
        char c = str[p], translation;
        if (c == '\\') {
            if (p >= str.length() - 2) {
                return false;
            }
            int sixteens, ones;
            if (!hex_to_int(str[p+1], &sixteens)) return false;
            if (!hex_to_int(str[p+2], &ones)) return false;
            translation = (sixteens * 16) | ones;
            p += 3;
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            translation = c;
            p++;
        } else {
            return false;
        }
        if (out->size() < MAX_KEY_SIZE) {
            out->contents()[out->size()] = translation;
            out->set_size(out->size() + 1);
        } else {
            return false;
        }
    }
    return true;
}

std::string key_to_cli_str(const store_key_t &key) {
    if (key == store_key_t::min()) {
        return "-inf";
    }
    std::string s;
    for (int i = 0; i < key.size(); i++) {
        uint8_t c = key.contents()[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            s.push_back(c);
        } else {
            s.push_back('\\');
            s.push_back(int_to_hex((c & 0xf0) >> 4));
            s.push_back(int_to_hex(c & 0x0f));
        }
    }
    return s;
}

std::string key_range_to_cli_str(const key_range_t &range) {
    if (range.right.unbounded) {
        return key_to_cli_str(range.left) + "-+inf";
    } else {
        return key_to_cli_str(range.left) + "-" + key_to_cli_str(range.right.key);
    }
}
