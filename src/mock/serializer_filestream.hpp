// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef MOCK_SERIALIZER_FILESTREAM_HPP_
#define MOCK_SERIALIZER_FILESTREAM_HPP_

#include "buffer_cache/types.hpp"
#include "containers/archive/archive.hpp"
#include "containers/scoped.hpp"
#include "errors.hpp"

class serializer_t;

namespace mock {

// These take a serializer and pretend that it's a file of fixed
// length.  It uses the first block to store the length and the rest
// of the blocks as blocks.

class serializer_file_read_stream_t : public read_stream_t {
public:
    explicit serializer_file_read_stream_t(serializer_t *serializer);
    ~serializer_file_read_stream_t();

    MUST_USE int64_t read(void *p, int64_t n);

private:
    serializer_t *serializer_;
    scoped_ptr_t<cache_t> cache_;
    int64_t known_size_;
    int64_t position_;

    DISABLE_COPYING(serializer_file_read_stream_t);
};

class serializer_file_write_stream_t : public write_stream_t {
public:
    // Truncates the file upon opening.
    explicit serializer_file_write_stream_t(serializer_t *serializer);
    ~serializer_file_write_stream_t();

    MUST_USE int64_t write(const void *p, int64_t n);

private:
    void write_size(int64_t size, transaction_t *txn);

    serializer_t *serializer_;
    scoped_ptr_t<cache_t> cache_;
    int64_t size_;

    DISABLE_COPYING(serializer_file_write_stream_t);
};


}  // namespace mock

#endif  // MOCK_SERIALIZER_FILESTREAM_HPP_
