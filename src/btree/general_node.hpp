#ifndef __BTREE_GENERAL_NODE_HPP__
#define __BTREE_GENERAL_NODE_HPP__

class block_magic_t;

namespace btree {

class value_sizer_t {
public:
    // The number of bytes the value takes up.  Reference implementation:
    //
    // for (int i = 0; i < INT_MAX; ++i) {
    //    if (fits(value, i)) return i;
    // }
    virtual int size(const char *value) = 0;

    // True if size(value) would return no more than length_available.
    // Does not read any bytes outside of [value, value +
    // length_available).
    virtual bool fits(const char *value, int length_available) = 0;

    // The magic that should be used for btree leaf nodes (or general
    // nodes) with this kind of value.
    virtual block_magic_t btree_leaf_magic() const = 0;
protected:
    virtual ~value_sizer_t() { }
};









}  // namespace btree

#endif  // __BTREE_GENERAL_NODE_HPP__
