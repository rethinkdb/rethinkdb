// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/keys.hpp"

#include "debug.hpp"
#include "utils.hpp"

std::string key_range_t::print() const {
    printf_buffer_t buf;
    debug_print(&buf, *this);
    return buf.c_str();
}

// fast-ish non-null terminated string comparison
int sized_strcmp(const uint8_t *str1, int len1, const uint8_t *str2, int len2) {
    int min_len = std::min(len1, len2);
    int res = memcmp(str1, str2, min_len);
    if (res == 0) {
        res = len1 - len2;
    }
    return res;
}

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

std::string key_to_debug_str(const btree_key_t *key) {
    return key_to_debug_str(store_key_t(key));
}

key_range_t::key_range_t() :
    left(), right(store_key_t()) { }

key_range_t::key_range_t(bound_t lm, const store_key_t& l, bound_t rm, const store_key_t& r) {
    init(lm, l.btree_key(), rm, r.btree_key());
}

key_range_t::key_range_t(bound_t lm, const btree_key_t *l, bound_t rm, const btree_key_t *r) {
    init(lm, l, rm, r);
}

void key_range_t::init(bound_t lm, const btree_key_t *l, bound_t rm, const btree_key_t *r) {
    switch (lm) {
        case closed:
            left.assign(l);
            break;
        case open:
            left.assign(l);
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
            left = store_key_t::min();
            break;
        default:
            unreachable();
    }

    switch (rm) {
        case closed: {
            right.unbounded = false;
            right.key().assign(r);
            bool ok = right.increment();
            guarantee(ok);
            break;
        }
        case open:
            right.unbounded = false;
            right.key().assign(r);
            break;
        case none:
            right.unbounded = true;
            break;
        default:
            unreachable();
    }

    rassert(right.unbounded || left <= right.key(),
            "left_key(%d)=%.*s, right_key(%d)=%.*s",
            left.size(), left.size(), left.contents(),
            right.internal_key.size(), right.internal_key.size(),
            right.internal_key.contents());
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

void debug_print(printf_buffer_t *buf, const btree_key_t *k) {
    if (k != nullptr) {
        debug_print_quoted_string(buf, k->contents, k->size);
    } else {
        buf->appendf("NULL");
    }
}

void debug_print(printf_buffer_t *buf, const store_key_t &k) {
    debug_print(buf, k.btree_key());
}

void debug_print(printf_buffer_t *buf, const key_range_t::right_bound_t &rb) {
    if (rb.unbounded) {
        buf->appendf("+inf");
    } else {
        debug_print(buf, rb.key());
    }
}

void debug_print(printf_buffer_t *buf, const key_range_t &kr) {
    buf->appendf("[");
    debug_print(buf, kr.left);
    buf->appendf(", ");
    debug_print(buf, kr.right);
    buf->appendf(")");
}

std::string key_range_to_string(const key_range_t &kr) {
    std::string res;
    res += "[";
    res += key_to_debug_str(kr.left);
    res += ", ";
    if (kr.right.unbounded) {
        res += "+inf";
    } else {
        res += key_to_debug_str(kr.right.key());
    }
    res += ")";
    return res;
}

void debug_print(printf_buffer_t *buf, const store_key_t *k) {
    if (k) {
        debug_print(buf, *k);
    } else {
        buf->appendf("NULL");
    }
}


bool operator==(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    return (a.unbounded && b.unbounded)
        || (!a.unbounded && !b.unbounded && a.key() == b.key());
}
bool operator!=(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    return !(a == b);
}
bool operator<(const key_range_t::right_bound_t &a, const key_range_t::right_bound_t &b) {
    if (a.unbounded) return false;
    if (b.unbounded) return true;
    return a.key() < b.key();
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

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_13(key_range_t::right_bound_t, unbounded, internal_key);
RDB_IMPL_SERIALIZABLE_2_SINCE_v1_13(key_range_t, left, right);

void serialize_for_metainfo(write_message_t *wm, const key_range_t &kr) {
    kr.left.serialize_for_metainfo(wm);
    serialize_universal(wm, kr.right.unbounded);
    kr.right.internal_key.serialize_for_metainfo(wm);
}
archive_result_t deserialize_for_metainfo(read_stream_t *s, key_range_t *out) {
    archive_result_t res = out->left.deserialize_for_metainfo(s);
    if (bad(res)) { return res; }
    res = deserialize_universal(s, &out->right.unbounded);
    if (bad(res)) { return res; }
    return out->right.internal_key.deserialize_for_metainfo(s);
}
