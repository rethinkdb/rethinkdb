#ifndef CONTAINERS_ARCHIVE_VERSIONED_HPP_
#define CONTAINERS_ARCHIVE_VERSIONED_HPP_

#include "containers/archive/archive.hpp"

// RSI: Rename to archive_version_t?  An enumeration of the current and previous
// recognized versions.  Every serialization / deserialization happens relative to
// one of these versions.
enum class cluster_version_t {
    v1_13,
    // See CLUSTER_VERSION, which should always be the latest version.
};

// Serializes a value for a given version.  DOES NOT SERIALIZE THE VERSION NUMBER!
template <class T>
void serialize_for_version(cluster_version_t version, write_message_t *wm,
                           const T &value) {
    // Right now, since there's only one version number, we can just call the normal
    // serialization function.
    rassert(version == cluster_version_t::v1_13);
    serialize(wm, value);
}

// Deserializes a value, assuming it's serialized for a given version.  (This doesn't
// deserialize any version numbers.)
template <class T>
archive_result_t deserialize_for_version(cluster_version_t version,
                                         read_stream_t *s,
                                         T *thing) {
    // Right now, since there's only one version number, we can just call the normal
    // serialization function.
    rassert(version == cluster_version_t::v1_13);
    deserialize(s, thing);
}



#endif  // CONTAINERS_ARCHIVE_VERSIONED_HPP_
