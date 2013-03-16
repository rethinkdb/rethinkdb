// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "btree/keys.hpp"

#include <algorithm>

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
                rassert(rm == none);
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

    rassert(right.unbounded || left <= right.key, "left_key=%.*s, right_key=%.*s", left.size(), left.contents(), right.key.size(), right.key.contents());
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


bool compare_range_by_left(const key_range_t &r1, const key_range_t &r2) {
    return r1.left < r2.left;
}

region_join_result_t region_join(const std::vector<key_range_t> &vec, key_range_t *out) THROWS_NOTHING {
    if (vec.empty()) {
        *out = key_range_t::empty();
        return REGION_JOIN_OK;
    } else {
        std::vector<key_range_t> sorted = vec;
        std::sort(sorted.begin(), sorted.end(), &compare_range_by_left);
        key_range_t::right_bound_t cursor = key_range_t::right_bound_t(sorted[0].left);
        for (size_t i = 0; i < sorted.size(); ++i) {
            if (cursor < key_range_t::right_bound_t(sorted[i].left)) {
                return REGION_JOIN_BAD_REGION;
            } else if (cursor > key_range_t::right_bound_t(sorted[i].left)) {
                return REGION_JOIN_BAD_JOIN;
            } else {
                /* The regions match exactly; move on to the next one. */
                cursor = sorted[i].right;
            }
        }
        key_range_t key_union;
        key_union.left = sorted[0].left;
        key_union.right = cursor;
        *out = key_union;
        return REGION_JOIN_OK;
    }
}

std::vector<key_range_t> region_subtract_many(key_range_t minuend, const std::vector<key_range_t>& subtrahends) {
    std::vector<key_range_t> buf, temp_result_buf;
    buf.push_back(minuend);
    for (std::vector<key_range_t>::const_iterator s = subtrahends.begin(); s != subtrahends.end(); ++s) {
        for (std::vector<key_range_t>::const_iterator m = buf.begin(); m != buf.end(); ++m) {
            // We are computing m-s here for each m in buf and s in subtrahends:
            // m-s = m & not(s) = m & ([-inf, s.left) | [s.right, +inf))
            //                  = (m & [-inf, s.left)) | (m & [s.right, +inf))
            key_range_t left = region_intersection(*m, key_range_t(key_range_t::none, store_key_t(), key_range_t::open, (*s).left));
            if (!left.is_empty()) {
                temp_result_buf.push_back(left);
            }
            if (!s->right.unbounded) {
                key_range_t right = region_intersection(*m, key_range_t(key_range_t::closed, s->right.key, key_range_t::none, store_key_t()));

                if (!right.is_empty()) {
                    temp_result_buf.push_back(right);
                }
            }
        }
        buf.swap(temp_result_buf);
        temp_result_buf.clear();
    }
    return buf;
}
