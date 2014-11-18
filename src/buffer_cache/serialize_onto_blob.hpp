#ifndef BUFFER_CACHE_SERIALIZE_ONTO_BLOB_HPP_
#define BUFFER_CACHE_SERIALIZE_ONTO_BLOB_HPP_

#include "buffer_cache/alt.hpp"
#include "buffer_cache/blob.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/buffer_group_stream.hpp"
#include "containers/archive/versioned.hpp"
#include "version.hpp"

void write_onto_blob(buf_parent_t parent, blob_t *blob,
                     const write_message_t &wm);

template <cluster_version_t W, class T>
void serialize_onto_blob(buf_parent_t parent, blob_t *blob,
                         const T &value) {
    // It _should_ never make sense to serialize an earlier version onto a blob.
    // Instead, you should probably update the version tag for the thing that you are
    // serializing (often stored in a block magic somewhere) and serialize with the
    // latest version.  See cluster_metadata_superblock_t in persist.cc for an
    // example -- things that modify the blobs call bring_up_to_date (which brings
    // everything serialized up to the latest version, if necessary) and then
    // serialize with the 'LATEST' version.
    //
    // The reason this type parameter exists at all is to make sure the person
    // writing the code on the calling end has their head in gear.
    static_assert(W == cluster_version_t::LATEST_DISK,
                  "It never makes sense to statically serialize an earlier version "
                  "onto a blob.  (Or does it?)");

    // We still make an unnecessary copy: serializing to a write_message_t instead of
    // directly onto the stream.  (However, don't be so sure it would be more
    // efficient to serialize onto an abstract stream type -- you've got a whole
    // bunch of virtual function calls that way.  But we do _deserialize_ off an
    // abstract stream type already, so what's the big deal?)
    write_message_t wm;
    serialize<W>(&wm, value);
    write_onto_blob(parent, blob, wm);
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



#endif  // BUFFER_CACHE_SERIALIZE_ONTO_BLOB_HPP_
