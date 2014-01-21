#ifndef BUFFER_CACHE_SERIALIZE_ONTO_BLOB_HPP_
#define BUFFER_CACHE_SERIALIZE_ONTO_BLOB_HPP_

#include "containers/archive/archive.hpp"
#include "containers/archive/buffer_group_stream.hpp"

template <class T>
void deserialize_from_group(const const_buffer_group_t *group, T *value_out) {
    buffer_group_read_stream_t stream(group);
    archive_result_t res = deserialize(&stream, value_out);
    guarantee_deserialization(res, "T (from a buffer group)");
    guarantee(stream.entire_stream_consumed(),
              "Corrupted value in storage (deserialization terminated early).");
};


#endif  // BUFFER_CACHE_SERIALIZE_ONTO_BLOB_HPP_
