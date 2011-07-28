#include "server/nested_demo/redis_utils.hpp"

#include "config/args.hpp"

namespace redis_utils {
    /* Constructs a btree_key_t from an std::string and puts it into out_buf */
    void construct_key(const std::string &key, scoped_malloc<btree_key_t> *out_buf) {
        rassert(key.length() <= MAX_KEY_SIZE);
        scoped_malloc<btree_key_t> tmp(offsetof(btree_key_t, contents) + key.length());
        out_buf->swap(tmp);
        (*out_buf)->size = key.length();
        memcpy((*out_buf)->contents, key.data(), key.length());
    }

    /* Conversion between int and a lexicographical string representation of those */
    #ifndef NDEBUG
    void check_int_representation() {
        // Assert that ints are stored in the right representation on this architecture...
        // (specifically that they are using 2's complement for negatives)
        rassert((unsigned int)((int)1) == (unsigned int)0x00000001u);
        rassert((unsigned int)((int)-1) == (unsigned int)0xffffffffu);
        rassert(sizeof(int) == 4);
        // Also check the endianess
        int check = 1;
        rassert((reinterpret_cast<char*>(&check))[0] == (char)1 && (reinterpret_cast<char*>(&check))[sizeof(int)-1] == (char)0, "Wrong endianess (not little).");
    }
    #endif

    void to_lex_int(const int i, char *buf) {
    #ifndef NDEBUG
        check_int_representation();
    #endif

        // Flip the sign bit, so positive values are larger than negative ones
        int i_flipped_sign = i ^ 0x80000000;

        // We are on a little endian architecture, so revert the bytes
        for (size_t p = 0; p < sizeof(int); ++p) {
            buf[sizeof(int) - p - 1] = (reinterpret_cast<char*>(&i_flipped_sign))[p];
        }
    }

    int from_lex_int(const char *buf) {
    #ifndef NDEBUG
        check_int_representation();
    #endif

        // Reverse to_lex_int()...
        int i_flipped_sign = 0;
        for (size_t p = 0; p < sizeof(int); ++p) {
            (reinterpret_cast<char*>(&i_flipped_sign))[sizeof(int) - p - 1] = buf[p];
        }
        int i = i_flipped_sign ^ 0x80000000;
        return i;
    }

    void to_lex_float(const float f, char *buf) {
        // The trick is as follows: IEEE 754 floats already are lexicographically
        // comparable, except for negative numbers, where the order is inverted
        // (e.g. -0.1 > -0.01). To work around this, we negate negative floats
        // after casting them to ints and then recover their sign bit to keep
        // comparison between positive and negative numbers intact.
        rassert(sizeof(int) == sizeof(float));
        int i = *reinterpret_cast<const int*>(&f);
        if (i < 0) {
            i = (-i) ^ 0x80000000;
        }
        to_lex_int(i, buf);
    }
    
    float from_lex_float(const char *buf) {
        rassert(sizeof(int) == sizeof(float));
        // TODO! Test this
        int i = from_lex_int(buf);
        if (i < 0) {
            i = -(i ^ 0x80000000);
        }
        return *reinterpret_cast<const float*>(&i);
    }
} /* namespace redis_utils */
