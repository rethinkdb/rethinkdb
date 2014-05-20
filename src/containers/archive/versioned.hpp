#ifndef CONTAINERS_ARCHIVE_VERSIONED_HPP_
#define CONTAINERS_ARCHIVE_VERSIONED_HPP_

#include "version.hpp"
#include "containers/archive/archive.hpp"

// This defines how to serialize cluster_version_t (which conveniently has a
// contiguous set of valid representation, from v1_13 to v1_13).
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(cluster_version_t, int8_t,
                                      cluster_version_t::v1_13,
                                      cluster_version_t::ONLY_VERSION);

// Serializes a value for a given version.  DOES NOT SERIALIZE THE VERSION NUMBER!
template <class T>
void serialize_for_version(DEBUG_VAR cluster_version_t version, write_message_t *wm,
                           const T &value) {
    // Right now, since there's only one version number, we can just call the normal
    // serialization function.
    rassert(version == cluster_version_t::ONLY_VERSION);
    serialize(wm, value);
}

// Deserializes a value, assuming it's serialized for a given version.  (This doesn't
// deserialize any version numbers.)
template <class T>
archive_result_t deserialize_for_version(DEBUG_VAR cluster_version_t version,
                                         read_stream_t *s,
                                         T *thing) {
    // Right now, since there's only one version number, we can just call the normal
    // serialization function.
    rassert(version == cluster_version_t::ONLY_VERSION);
    return deserialize(s, thing);
}



#endif  // CONTAINERS_ARCHIVE_VERSIONED_HPP_
