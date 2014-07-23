#ifndef RDB_PROTOCOL_SERIALIZE_DATUM_ONTO_BLOB_HPP_
#define RDB_PROTOCOL_SERIALIZE_DATUM_ONTO_BLOB_HPP_

#include "rdb_protocol/serialize_datum.hpp"

inline void datum_serialize_onto_blob(buf_parent_t parent, blob_t *blob,
                                      const counted_t<const ql::datum_t> &value) {
    // We still make an unnecessary copy: serializing to a write_message_t instead of
    // directly onto the stream.  (However, don't be so sure it would be more
    // efficient to serialize onto an abstract stream type -- you've got a whole
    // bunch of virtual function calls that way.  But we do _deserialize_ off an
    // abstract stream type already, so what's the big deal?)
    write_message_t wm;
    datum_serialize(&wm, value);
    write_onto_blob(parent, blob, wm);
}

inline void datum_deserialize_from_group(const const_buffer_group_t *group,
                                         counted_t<const ql::datum_t> *value_out) {
    buffer_group_read_stream_t stream(group);
    archive_result_t res = datum_deserialize(&stream, value_out);
    guarantee_deserialization(res, "datum_t (from a buffer group)");
    guarantee(stream.entire_stream_consumed(),
              "Corrupted value in storage (deserialization terminated early).");
};


inline void datum_deserialize_from_blob(buf_parent_t parent, blob_t *blob,
                                        counted_t<const ql::datum_t> *value_out) {
    buffer_group_t group;
    blob_acq_t acq;
    blob->expose_all(parent, access_t::read, &group, &acq);
    datum_deserialize_from_group(const_view(&group), value_out);
}


#endif /* RDB_PROTOCOL_SERIALIZE_DATUM_ONTO_BLOB_HPP_ */
