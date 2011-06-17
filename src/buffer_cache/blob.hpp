#ifndef __BUFFER_CACHE_BLOB_HPP__
#define __BUFFER_CACHE_BLOB_HPP__

#include <stdint.h>
#include <stddef.h>

class transaction_t;
class buffer_group_t;
class block_size_t;

class blob_t {
public:
    blob_t(block_size_t block_size, const char *ref, size_t maxreflen);
    void dump_ref(block_size_t block_size, char *ref_out, size_t confirm_maxreflen);
    ~blob_t();


    size_t refsize(block_size_t block_size) const;
    int64_t valuesize() const;

    void expose_region(transaction_t *txn, int64_t offset, int64_t size, buffer_group_t *buffer_group_out);
    void append_region(transaction_t *txn, int64_t size);
    void prepend_region(transaction_t *txn, int64_t size);

    void unappend_region(transaction_t *txn, int64_t size);
    void unprepend_region(transaction_t *txn, int64_t size);

private:
    char *ref_;
    size_t maxreflen_;

    // disable copying
    blob_t(const blob_t&);
    void operator=(const blob_t&);
};




#endif  // __BUFFER_CACHE_BLOB_HPP__
