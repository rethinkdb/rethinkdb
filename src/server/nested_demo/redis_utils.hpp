#ifndef __SERVER_NESTED_DEMO_REDIS_UTILS_HPP_
#define	__SERVER_NESTED_DEMO_REDIS_UTILS_HPP_

/* nested string value type */
// TODO! Replace this by a version that supports blobs!
struct redis_nested_string_value_t {
    int length;
    char contents[];

public:
    int inline_size(UNUSED block_size_t bs) const {
        return sizeof(length) + length;
    }

    int64_t value_size() const {
        return length;
    }

    const char *value_ref() const { return contents; }
    char *value_ref() { return contents; }
};
template <>
class value_sizer_t<redis_nested_string_value_t> {
public:
    value_sizer_t<redis_nested_string_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const redis_nested_string_value_t *value) const {
        return value->inline_size(block_size_);
    }

    bool fits(UNUSED const redis_nested_string_value_t *value, UNUSED int length_available) const {
        // TODO!
        return true;
    }

    int max_possible_size() const {
        // TODO?
        return MAX_BTREE_VALUE_SIZE;
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { { 'l', 'r', 'n', 's' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;
};


/* nested empty value type */
struct redis_nested_empty_value_t {
    int inline_size(UNUSED block_size_t bs) const {
        return 0;
    }

    int64_t value_size() const {
        return 0;
    }

    const char *value_ref() const { return NULL; }
    char *value_ref() { return NULL; }
};
template <>
class value_sizer_t<redis_nested_empty_value_t> {
public:
    value_sizer_t<redis_nested_empty_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const redis_nested_empty_value_t *value) const {
        return value->inline_size(block_size_);
    }

    bool fits(UNUSED const redis_nested_empty_value_t *value, UNUSED int length_available) const {
        return true;
    }

    int max_possible_size() const {
        return 0;
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { { 'l', 'r', 'n', 'e' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;
};

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

#define LEX_INT_SIZE (sizeof(int))

void to_lex_int(const int i, char* buf) {
#ifndef NDEBUG
    check_int_representation();
#endif

    // Flip the sign bit, so positive values are larger than negative ones
    int i_flipped_sign = i ^ 0x80000000;

    // We are on a little endian architecture, so revert the bytes
    for (size_t p = 0; p < sizeof(int); ++p) {
        buf[sizeof(int) - p] = (reinterpret_cast<char*>(&i_flipped_sign))[p];
    }
}

int from_lex_int(char* buf) {
#ifndef NDEBUG
    check_int_representation();
#endif

    // Reverse to_lex_int()...
    int i_flipped_sign = 0;
    for (size_t p = 0; p < sizeof(int); ++p) {
        (reinterpret_cast<char*>(i_flipped_sign))[sizeof(int) - p] = buf[p];
    }
    int i = i_flipped_sign ^ 0x80000000;
    return i;
}

// TODO! (later) lex. float key representation
static const size_t LEX_FLOAT_SIZE = 0;
void to_lex_float(UNUSED const float f, UNUSED char* buf) {
    // TODO!
}
float from_lex_float(UNUSED char* buf) {
    // TODO!
    return 0.0f;
}

#endif	/* __SERVER_NESTED_DEMO_REDIS_UTILS_HPP_ */

