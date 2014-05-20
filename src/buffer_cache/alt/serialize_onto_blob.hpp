#ifndef BUFFER_CACHE_ALT_SERIALIZE_ONTO_BLOB_HPP_
#define BUFFER_CACHE_ALT_SERIALIZE_ONTO_BLOB_HPP_

#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/blob.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/buffer_group_stream.hpp"
#include "containers/archive/versioned.hpp"
#include "version.hpp"

void write_onto_blob(buf_parent_t parent, blob_t *blob,
                     const write_message_t &wm);

template <class T>
void serialize_onto_blob(buf_parent_t parent, blob_t *blob,
                         const T &value) {
    // We still make an unnecessary copy: serializing to a write_message_t instead of
    // directly onto the stream.  (However, don't be so sure it would be more
    // efficient to serialize onto an abstract stream type -- you've got a whole
    // bunch of virtual function calls that way.  But we do _deserialize_ off an
    // abstract stream type already, so what's the big deal?)
    write_message_t wm;
    serialize(&wm, value);
    write_onto_blob(parent, blob, wm);
}

template <class T>
void serialize_for_version_onto_blob(cluster_version_t cluster_version,
                                     buf_parent_t parent, blob_t *blob,
                                     const T &value) {
    // We still make an unnecessary copy: see above.
    write_message_t wm;
    serialize_for_version(cluster_version, &wm, value);
    write_onto_blob(parent, blob, wm);
}

template <class T>
void deserialize_from_blob(buf_parent_t parent, blob_t *blob,
                           T *value_out) {
    buffer_group_t group;
    blob_acq_t acq;
    blob->expose_all(parent, access_t::read, &group, &acq);
    deserialize_from_group(const_view(&group), value_out);
}

template <class T>
void deserialize_for_version_from_blob(cluster_version_t cluster_version,
                                       buf_parent_t parent, blob_t *blob,
                                       T *value_out) {
    buffer_group_t group;
    blob_acq_t acq;
    blob->expose_all(parent, access_t::read, &group, &acq);
    deserialize_for_version_from_group(cluster_version, const_view(&group), value_out);
}



#endif  // BUFFER_CACHE_ALT_SERIALIZE_ONTO_BLOB_HPP_
