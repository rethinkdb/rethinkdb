#include "compression/blob_adapter.hpp"

blob_sink_t::blob_sink_t(blob_t *_internal, transaction_t *_txn)
    : internal(_internal), txn(_txn) { }

void blob_sink_t::Append(const char *bytes, size_t n) {
    int64_t offset = internal->valuesize();
    internal->append_region(txn, n);
    internal->write_from_string(bytes, n, txn, offset);
}
