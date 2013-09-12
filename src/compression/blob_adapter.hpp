#ifndef COMPRESSION_BLOB_ADAPTER_T_
#define COMPRESSION_BLOB_ADAPTER_T_

#include <snappy-sinksource.h>
#include "buffer_cache/blob.hpp"

class blob_sink_t : public snappy::Sink {
public:
    blob_sink_t(blob_t *internal, transaction_t *txn);
    virtual void Append(const char *bytes, size_t n);

private:
    blob_t *internal;
    transaction_t *txn;
};

void compress_to_blob(blob_t *blob, transaction_t *txn, const std::string &data);

class blob_source_t : public snappy::Source {
public:
    blob_source_t(blob_t *internal, transaction_t *txn);

    virtual size_t Available() const;

    virtual const char* Peek(size_t* len);

    virtual void Skip(size_t n);

private:
    blob_t *internal;
    transaction_t *txn;
    buffer_group_t buf_group;
    blob_acq_t blob_acq;
    size_t size_remaining, buf_offset, buf_num;
};

void decompress_from_blob(blob_t *blob, transaction_t *txn, std::vector<char> *data_out);

#endif
