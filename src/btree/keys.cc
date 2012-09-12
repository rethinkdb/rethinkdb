#include "btree/keys.hpp"

bool unescaped_str_to_key(const char *str, int len, store_key_t *buf) {
    if (len <= MAX_KEY_SIZE) {
        memcpy(buf->contents(), str, len);
        buf->set_size(uint8_t(len));
        return true;
    } else {
        return false;
    }
}

std::string key_to_unescaped_str(const store_key_t &key) {
    return std::string(reinterpret_cast<const char *>(key.contents()), key.size());
}

std::string key_to_debug_str(const store_key_t &key) {
    std::string s;
    s.push_back('"');
    for (int i = 0; i < key.size(); i++) {
        uint8_t c = key.contents()[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            s.push_back(c);
        } else {
            s.push_back('\\');
            s.push_back('x');
            s.push_back(int_to_hex((c & 0xf0) >> 4));
            s.push_back(int_to_hex(c & 0x0f));
        }
    }
    s.push_back('"');
    return s;
}

key_range_t::key_range_t() :
    left(), right(store_key_t()) { }

key_range_t::key_range_t(bound_t lm, const store_key_t& l, bound_t rm, const store_key_t& r) {
    switch (lm) {
        case closed:
            left = l;
            break;
        case open:
            left = l;
            if (left.increment()) {
                break;
            } else {
                /* Our left bound is the largest possible key, and we are open
                on the left-hand side. So we are empty. */
                *this = key_range_t::empty();
                return;
            }
        case none:
            left = store_key_t();
            break;
        default:
            unreachable();
    }

    switch (rm) {
        case closed: {
            store_key_t r_copy(r);
            if (r_copy.increment()) {
                right = right_bound_t(r_copy);
                break;
            } else {
                /* Our right bound is the largest possible key, and we are
                closed on the right-hand side. The only way to express this is
                to set `right` to `right_bound_t()`. */
                right = right_bound_t();
                break;
            }
        }
        case open:
            right = right_bound_t(r);
            break;
        case none:
            right = right_bound_t();
            break;
        default:
            unreachable();
    }
}

bool key_range_t::is_superset(const key_range_t &other) const {
    /* Special-case empty ranges */
    if (other.is_empty()) return true;
    if (left > other.left) return false;
    if (right < other.right) return false;
    return true;
}

bool key_range_t::overlaps(const key_range_t &other) const {
    return key_range_t::right_bound_t(left) < other.right &&
        key_range_t::right_bound_t(other.left) < right &&
        !is_empty() && !other.is_empty();
}

key_range_t key_range_t::intersection(const key_range_t &other) const {
    if (!overlaps(other)) {
        return key_range_t::empty();
    }
    key_range_t ixn;
    ixn.left = left < other.left ? other.left : left;
    ixn.right = right > other.right ? other.right : right;
    return ixn;
}

std::string key_range_to_debug_str(const key_range_t &kr) {
    std::string ret;
    ret += "[";
    ret += key_to_debug_str(kr.left);
    ret += ", ";
    if (kr.right.unbounded) {
        ret += "+inf)";
    } else {
        ret += key_to_debug_str(kr.right.key);
        ret += ")";
    }
    return ret;
}

void debug_print(append_only_printf_buffer_t *buf, const store_key_t &k) {
    debug_print_quoted_string(buf, k.contents(), k.size());
}

void debug_print(append_only_printf_buffer_t *buf, const key_range_t &kr) {
    buf->appendf("[");
    debug_print(buf, kr.left);
    buf->appendf(", ");
    if (kr.right.unbounded) {
        buf->appendf("+inf");
    } else {
        debug_print(buf, kr.right.key);
    }
    buf->appendf(")");
}

void debug_print(append_only_printf_buffer_t *buf, const store_key_t *k) {
    if (k) {
        debug_print(buf, *k);
    } else {
        buf->appendf("NULL");
    }
}


bool operator==(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    return a.unbounded == b.unbounded && a.key == b.key;
}
bool operator!=(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    return !(a == b);
}
bool operator<(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    if (a.unbounded) return false;
    if (b.unbounded) return true;
    return a.key < b.key;
}
bool operator<=(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    return a == b || a < b;
}
bool operator>(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    return b < a;
}
bool operator>=(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    return b <= a;
}

bool operator==(key_range_t a, key_range_t b) THROWS_NOTHING {
    return a.left == b.left && a.right == b.right;
}

bool operator!=(key_range_t a, key_range_t b) THROWS_NOTHING {
    return !(a == b);
}

bool operator<(const key_range_t &a, const key_range_t &b) THROWS_NOTHING {
    return (a.left < b.left || (a.left == b.left && a.right < b.right));
}

RDB_IMPL_SERIALIZABLE_2(key_range_t::right_bound_t, unbounded, key);
RDB_IMPL_SERIALIZABLE_2(key_range_t, left, right);
