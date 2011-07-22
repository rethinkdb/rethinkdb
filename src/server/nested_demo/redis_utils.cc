#include "server/nested_demo/redis_utils.hpp"

namespace redis_utils {
    /* Constructs a btree_key_t from an std::string and puts it into out_buf */
    void construct_key(const std::string &key, scoped_malloc<btree_key_t> *out_buf) {
        scoped_malloc<btree_key_t> tmp(offsetof(btree_key_t, contents) + key.length());
        out_buf->swap(tmp);
        (*out_buf)->size = key.length();
        memcpy((*out_buf)->contents, key.data(), key.length());
    }

    /* Convenience accessor functions for nested trees */
    void find_nested_keyvalue_location_for_write(block_size_t block_size, boost::shared_ptr<transaction_t> &transaction, const std::string &field, repli_timestamp_t ts, keyvalue_location_t<redis_nested_string_value_t> *kv_location, scoped_malloc<btree_key_t> *btree_key, const block_id_t nested_root) {
        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(nested_root));

        // Construct a sizer for the sub tree, using the same block size as the super tree
        value_sizer_t<redis_nested_string_value_t> sizer(block_size);

        got_superblock_t got_superblock;
        got_superblock.sb.swap(nested_btree_sb);
        got_superblock.txn = transaction;

        // Construct the key
        construct_key(field, btree_key);

        // Find the element
        ::find_keyvalue_location_for_write(&sizer, &got_superblock, btree_key->get(), ts, kv_location);
    }
    void find_nested_keyvalue_location_for_read(block_size_t block_size, boost::shared_ptr<transaction_t> &transaction, const std::string &field, keyvalue_location_t<redis_nested_string_value_t> *kv_location, const block_id_t nested_root) {
        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(nested_root));

        // Construct a sizer for the sub tree, using the same block size as the super tree
        value_sizer_t<redis_nested_string_value_t> sizer(block_size);

        got_superblock_t got_superblock;
        got_superblock.sb.swap(nested_btree_sb);
        got_superblock.txn = transaction;

        // Construct the key
        scoped_malloc<btree_key_t> btree_key;
        construct_key(field, &btree_key);

        // Find the element
        ::find_keyvalue_location_for_read(&sizer, &got_superblock, btree_key.get(), kv_location);
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
    void to_lex_float(UNUSED const float f, UNUSED char* buf) {
        // TODO!
    }
    float from_lex_float(UNUSED char* buf) {
        // TODO!
        return 0.0f;
    }
} /* namespace redis_utils */
