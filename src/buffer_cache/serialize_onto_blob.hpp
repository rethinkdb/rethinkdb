#ifndef BUFFER_CACHE_SERIALIZE_TO_BLOB_HPP_
#define BUFFER_CACHE_SERIALIZE_TO_BLOB_HPP_

#include "buffer_cache/blob.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/vector_stream.hpp"

template <class T>
void serialize_onto_blob(transaction_t *txn, blob_t *blob, const T &value) {
    // RSI: This is the inefficient implementation.  Make the efficient one.
    write_message_t wm;
    wm << value;
    vector_stream_t stream;
    int res = send_write_message(&stream, &wm);
    guarantee(res == 0);

    blob->clear(txn);
    blob->append_region(txn, stream.vector().size());
    std::string sered_data(stream.vector().begin(), stream.vector().end());
    blob->write_from_string(sered_data, txn, 0);
}



#endif  // BUFFER_CACHE_SERIALIZE_TO_BLOB_HPP_
