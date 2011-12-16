#ifndef __BTREE_KEYS_HPP__
#define __BTREE_KEYS_HPP__

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/serialization/binary_object.hpp>

#include "config/args.hpp"
#include "protocol_api.hpp" // FIXME: ugh!
#include "utils.hpp"

struct store_key_t {
    uint8_t size;
    char contents[MAX_KEY_SIZE];

    store_key_t() : size(0) { }

    store_key_t(int sz, const char *buf) {
        assign(sz, buf);
    }

    store_key_t(const store_key_t& key_) {
        assign(key_.size, key_.contents);
    }

    explicit store_key_t(const std::string& s) {
        assign(s.size(), s.data());
    }

    void assign(int sz, const char *buf) {
        rassert(sz <= MAX_KEY_SIZE);
        size = sz;
        memcpy(contents, buf, sz);
    }

    void print() const {
        printf("%*.*s", size, size, contents);
    }

    static store_key_t min() {
        return store_key_t(0, NULL);
    }

    static store_key_t max() {
        uint8_t buf[MAX_KEY_SIZE];
        for (int i = 0; i < MAX_KEY_SIZE; i++) {
            buf[i] = 255;
        }
        return store_key_t(MAX_KEY_SIZE, reinterpret_cast<char *>(buf));
    }

    bool increment() {
        if (size < MAX_KEY_SIZE) {
            contents[size] = 0;
            size++;
            return true;
        }
        while (size > 0 && (reinterpret_cast<uint8_t *>(contents))[size-1] == 255) {
            size--;
        }
        if (size == 0) {
            /* We were the largest possible key. Oops. Restore our previous
            state and return `false`. */
            *this = store_key_t::max();
            return false;
        }
        (reinterpret_cast<uint8_t *>(contents))[size-1]++;
        return true;
    }

    bool decrement() {
        if (size == 0) {
            return false;
        }
        if ((reinterpret_cast<uint8_t *>(contents))[size-1] > 0) {
            (reinterpret_cast<uint8_t *>(contents))[size-1]--;
            return true;
        }
        size--;
        return true;
    }

    int compare(const store_key_t& k) const {
        return sized_strcmp(contents, size, k.contents, k.size);
    }

    friend class boost::serialization::access;
    template<typename Archive> void serialize(Archive &ar, UNUSED const unsigned int version) {
        ar & size;
        ar & boost::serialization::make_binary_object(contents, size);
    }
};

inline bool operator==(const store_key_t &k1, const store_key_t &k2) {
    return k1.size == k2.size && memcmp(k1.contents, k2.contents, k1.size) == 0;
}

inline bool operator!=(const store_key_t &k1, const store_key_t &k2) {
    return !(k1 == k2);
}

inline bool operator<(const store_key_t &k1, const store_key_t &k2) {
    return k1.compare(k2) < 0;
}

inline bool operator>(const store_key_t &k1, const store_key_t &k2) {
    return k2 < k1;
}

inline bool operator<=(const store_key_t &k1, const store_key_t &k2) {
    return k1.compare(k2) <= 0;
}

inline bool operator>=(const store_key_t &k1, const store_key_t &k2) {
    return k2 <= k1;
}

inline bool str_to_key(const char *str, store_key_t *buf) {
    int len = strlen(str);
    if (len <= MAX_KEY_SIZE) {
        memcpy(buf->contents, str, len);
        buf->size = (uint8_t) len;
        return true;
    } else {
        return false;
    }
}

inline std::string key_to_str(const store_key_t &key) {
    return std::string(key.contents, key.size);
}

/* `key_range_t` represents a contiguous set of keys. */
struct key_range_t {
    enum bound_t {
        open,
        closed,
        none
    };

    key_range_t();   /* creates a range containing no keys */
    key_range_t(bound_t, const store_key_t&, bound_t, const store_key_t&);

    static key_range_t empty() THROWS_NOTHING {
        return key_range_t();
    }

    bool is_empty() const {
        return !right.unbounded && left <= right.key;
    }

    bool contains_key(const store_key_t& key) const {
        bool left_ok = left <= key;
        bool right_ok = right.unbounded || key < right.key;
        return left_ok && right_ok;
    }

    bool contains_key(const char *key, uint8_t size) const {
        bool left_ok = sized_strcmp(left.contents, left.size, key, size) <= 0;
        bool right_ok = right.unbounded || sized_strcmp(key, size, right.key.contents, right.key.size) < 0;
        return left_ok && right_ok;
    }

    /* If `right.unbounded`, then the range contains all keys greater than or
    equal to `left`. If `right.bounded`, then the range contains all keys
    greater than or equal to `left` and less than `right.key`. */
    struct right_bound_t {
        right_bound_t() : unbounded(true) { }
        explicit right_bound_t(store_key_t k) : unbounded(false), key(k) { }
        bool unbounded;
        store_key_t key;

        RDB_MAKE_ME_SERIALIZABLE_2(unbounded, key);
    };
    store_key_t left;
    right_bound_t right;

    RDB_MAKE_ME_SERIALIZABLE_2(left, right);
};

bool region_is_superset(const key_range_t &potential_superset, const key_range_t &potential_subset) THROWS_NOTHING;
key_range_t region_intersection(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING;
key_range_t region_join(const std::vector<key_range_t> &vec) THROWS_ONLY(bad_join_exc_t, bad_region_exc_t);
bool region_overlaps(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING;
std::vector<key_range_t> region_subtract_many(key_range_t a, const std::vector<key_range_t>& b);

bool operator==(key_range_t, key_range_t) THROWS_NOTHING;
bool operator!=(key_range_t, key_range_t) THROWS_NOTHING;

#endif // __BTREE_KEYS_HPP__
