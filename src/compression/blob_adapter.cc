#include "compression/blob_adapter.hpp"

blob_sink_t::blob_sink_t(blob_t *_internal, transaction_t *_txn)
    : internal(_internal), txn(_txn) { }

void blob_sink_t::Append(const char *bytes, size_t n) {
    int64_t offset = internal->valuesize();
    internal->append_region(txn, n);
    internal->write_from_string(bytes, n, txn, offset);
}


blob_source_t::blob_source_t(blob_t *_internal, transaction_t *_txn) 
    : internal(_internal), txn(_txn), buf_offset(0), buf_num(0) {
    internal->expose_all(txn, rwi_read, &buf_group, &blob_acq);
    size_remaining = buf_group.get_size();
}
    void expose_all(transaction_t *txn, access_t mode, buffer_group_t *buffer_group_out, blob_acq_t *acq_group_out);

size_t blob_source_t::Available() const {
    return size_remaining;
}


const char* blob_source_t::Peek(size_t* len) {
    guarantee(buf_num < buf_group.num_buffers());
    *len = buf_group.get_buffer(buf_num).size - buf_offset;
    guarantee(len > 0);
    return static_cast<char *>(buf_group.get_buffer(buf_num).data) + buf_offset;
}

void blob_source_t::Skip(size_t n) {
    guarantee(buf_num < buf_group.num_buffers());
    while ((buf_group.get_buffer(buf_num).size - buf_offset) < n) {
        buf_num++;
        n -= (buf_group.get_buffer(buf_num).size - buf_offset);
        buf_offset = 0;
    }

    buf_offset = n;
}
