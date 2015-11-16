// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef UNITTEST_BTREE_UTILS_HPP_
#define UNITTEST_BTREE_UTILS_HPP_

#include "btree/operations.hpp"
#include "containers/scoped.hpp"

struct short_value_t;

class short_value_sizer_t : public value_sizer_t {
public:
    explicit short_value_sizer_t(max_block_size_t bs) : block_size_(bs) { }

    int size(const void *value) const {
        int x = *reinterpret_cast<const uint8_t *>(value);
        return 1 + x;
    }

    bool fits(const void *value, int length_available) const {
        return length_available > 0 && size(value) <= length_available;
    }

    int max_possible_size() const {
        return 256;
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { { 's', 'h', 'L', 'F' } };
        return magic;
    }

    max_block_size_t block_size() const { return block_size_; }

private:
    max_block_size_t block_size_;

    DISABLE_COPYING(short_value_sizer_t);
};

class short_value_buffer_t {
public:
    explicit short_value_buffer_t(const short_value_t *v) {
        memcpy(data_, v, reinterpret_cast<const uint8_t *>(v)[0] + 1);
    }

    explicit short_value_buffer_t(const std::string& v) {
        rassert(v.size() <= 255);
        data_[0] = v.size();
        memcpy(data_ + 1, v.data(), v.size());
    }

    short_value_t *data() {
        return reinterpret_cast<short_value_t *>(data_);
    }

    size_t size() {
        return data_[0] + 1;
    }

    std::string as_str() const {
        return std::string(data_ + 1, data_ + 1 + data_[0]);
    }

private:
    uint8_t data_[256];
};

#endif /* UNITTEST_BTREE_UTILS_HPP_ */
