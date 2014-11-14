#include "buffer_cache/serialize_onto_blob.hpp"

void write_onto_blob(buf_parent_t parent, blob_t *blob,
                     const write_message_t &wm) {
    blob->clear(parent);
    blob->append_region(parent, wm.size());

    blob_acq_t acq;
    buffer_group_t group;
    blob->expose_all(parent, access_t::write, &group, &acq);

    buffer_group_write_stream_t stream(&group);
    int res = send_write_message(&stream, &wm);
    guarantee(res == 0,
              "Failed to put write_message_t into buffer group.  "
              "(Was the blob made too small?).");
    guarantee(stream.entire_stream_filled(),
              "Blob not filled by write_message_t (Was it made too big?)");
}

