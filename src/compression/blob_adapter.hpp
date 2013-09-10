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

class blob_source_t : public snappy::Source {
};

#endif
