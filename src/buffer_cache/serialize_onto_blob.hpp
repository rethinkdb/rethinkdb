#ifndef BUFFER_CACHE_SERIALIZE_ONTO_BLOB_HPP_
#define BUFFER_CACHE_SERIALIZE_ONTO_BLOB_HPP_

#include "buffer_cache/blob.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/buffer_group_stream.hpp"

void write_onto_blob(transaction_t *txn, blob_t *blob, const write_message_t &wm);

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
void deserialize_from_group(const const_buffer_group_t *group, T *value_out) {
    buffer_group_read_stream_t stream(group);
    archive_result_t res = deserialize(&stream, value_out);
    guarantee(res == 0, "Corrupted value in storage (couldn't deserialize).");
    guarantee(stream.entire_stream_consumed(),
              "Corrupted value in storage (deserialization terminated early).");
};

template <class T>
void deserialize_from_blob(transaction_t *txn, blob_t *blob, T *value_out) {
    buffer_group_t group;
    blob_acq_t acq;
    blob->expose_all(txn, rwi_read, &group, &acq);
    deserialize_from_group(const_view(&group), value_out);
}


#endif  // BUFFER_CACHE_SERIALIZE_ONTO_BLOB_HPP_
