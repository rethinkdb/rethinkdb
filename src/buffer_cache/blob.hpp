#ifndef __BUFFER_CACHE_BLOB_HPP__
#define __BUFFER_CACHE_BLOB_HPP__

#include <stdint.h>
#include <stddef.h>

class transaction_t;
class buffer_group_t;

class blob_t {
public:
    blob_t(transaction_t *txn, const char *ref, size_t maxreflen);
    void dump_ref(char *ref_out, size_t confirm_maxreflen);
    ~blob_t();


    size_t refsize() const;
    int64_t valuesize() const;

    buffer_group_t expose_region(int64_t offset, int64_t size);
    void insert_region(int64_t offset, int64_t size);
    void remove_region(int64_t offset, int64_t size);

private:
    char *ref_;
    size_t maxreflen_;

    // disable copying
    blob_t(const blob_t&);
    void operator=(const blob_t&);
};




#endif  // __BUFFER_CACHE_BLOB_HPP__
