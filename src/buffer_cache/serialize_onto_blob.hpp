#ifndef BUFFER_CACHE_SERIALIZE_TO_BLOB_HPP_
#define BUFFER_CACHE_SERIALIZE_TO_BLOB_HPP_

#include "buffer_cache/blob.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/vector_stream.hpp"

inline void write_onto_blob(transaction_t *txn, blob_t *blob, const write_message_t &wm) {
    vector_stream_t stream;
    int res = send_write_message(&stream, &wm);
    guarantee(res == 0);

    blob->clear(txn);
    blob->append_region(txn, stream.vector().size());
    std::string sered_data(stream.vector().begin(), stream.vector().end());
    blob->write_from_string(sered_data, txn, 0);
}

template <class T>
void serialize_onto_blob(transaction_t *txn, blob_t *blob, const T &value) {
    // RSI: This is the inefficient implementation.  Make the efficient one.
    write_message_t wm;
    wm << value;
    write_onto_blob(txn, blob, wm);
}

template <class T>
void deserialize_from_blob(transaction_t *txn, blob_t *blob, T *value_out) {
    // RSI: This is the inefficient implementation.  Make the efficient one.

    buffer_group_t group;
    blob_acq_t acq;
    blob->expose_all(txn, rwi_read, &group, &acq);

    const int64_t group_size(group.get_size());
    std::vector<char> vec(group_size);

    buffer_group_t group_cpy;
    group_cpy.add_buffer(group_size, vec.data());
    buffer_group_copy_data(&group_cpy, const_view(&group));

    vector_read_stream_t read_stream(&vec);
    archive_result_t res = deserialize(&read_stream, value_out);
    guarantee(res == 0, "Corrupted value in storage.");

    guarantee(read_stream.entire_stream_consumed());
}


#endif  // BUFFER_CACHE_SERIALIZE_TO_BLOB_HPP_
