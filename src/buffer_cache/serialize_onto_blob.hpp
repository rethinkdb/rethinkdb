#ifndef BUFFER_CACHE_SERIALIZE_TO_BLOB_HPP_
#define BUFFER_CACHE_SERIALIZE_TO_BLOB_HPP_

#include "buffer_cache/blob.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/buffer_group_stream.hpp"

inline void write_onto_blob(transaction_t *txn, blob_t *blob, const write_message_t &wm) {
    blob->clear(txn);
    blob->append_region(txn, wm.size());

    blob_acq_t acq;
    buffer_group_t group;
    blob->expose_all(txn, rwi_write, &group, &acq);

    buffer_group_write_stream_t stream(&group);
    int res = send_write_message(&stream, &wm);
    guarantee(res == 0,
              "Failed to put write_message_t into buffer group.  "
              "(Was the blob made too small?).");
    guarantee(stream.entire_stream_filled(),
              "Blob not filled by write_message_t (Was it made too big?)");
}

template <class T>
void serialize_onto_blob(transaction_t *txn, blob_t *blob, const T &value) {
    // We still make an unnecessary copy: serializing to a write_message_t instead of
    // directly onto the stream.  (However, don't be so sure it would be more efficient
    // to serialize onto an abstract stream type -- you've got a whole bunch of virtual
    // function calls that way.  But we do _deserialize_ off an abstract stream type
    // already, so what's the big deal?)
    write_message_t wm;
    wm << value;
    write_onto_blob(txn, blob, wm);
}

template <class T>
void deserialize_from_blob(transaction_t *txn, blob_t *blob, T *value_out) {
    buffer_group_t group;
    blob_acq_t acq;
    blob->expose_all(txn, rwi_read, &group, &acq);

    buffer_group_read_stream_t stream(const_view(&group));
    archive_result_t res = deserialize(&stream, value_out);
    guarantee(res == 0, "Corrupted value in storage (couldn't deserialize).");
    guarantee(stream.entire_stream_consumed(),
              "Corrupted value in storage (deserialization terminated early).");
}


#endif  // BUFFER_CACHE_SERIALIZE_TO_BLOB_HPP_
