#ifndef CONTAINERS_ARCHIVE_VERSIONED_HPP_
#define CONTAINERS_ARCHIVE_VERSIONED_HPP_

#include "version.hpp"
#include "containers/archive/archive.hpp"

// This defines how to serialize cluster_version_t (which conveniently has a
// contiguous set of valid representation, from v1_13 to v1_13).
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(cluster_version_t, int8_t,
                                      cluster_version_t::v1_13,
                                      cluster_version_t::ONLY_VERSION);

// RSI: Remove this.
// RSI: Rename vserialize to serialize.
// RSI: Rename vdeserialize to deserialize.
#if 1
template <cluster_version_t V, class T>
void vserialize(write_message_t *wm, const T &value) {
    CT_ASSERT(V == cluster_version_t::ONLY_VERSION);  // RSI
    serialize(wm, value);
}

template <cluster_version_t V, class T>
archive_result_t vdeserialize(read_stream_t *wm, T *value) {
    CT_ASSERT(V == cluster_version_t::ONLY_VERSION);  // RSI
    return deserialize(wm, value);
}
#endif  // RSI

// Serializes a value for a given version.  DOES NOT SERIALIZE THE VERSION NUMBER!
template <class T>
void serialize_for_version(DEBUG_VAR cluster_version_t version, write_message_t *wm,
                           const T &value) {
    // Right now, since there's only one version number, we can just call the normal
    // serialization function.
    rassert(version == cluster_version_t::ONLY_VERSION);
    switch (version) {
    case cluster_version_t::v1_13:
        vserialize<cluster_version_t::v1_13>(wm, value);
    }
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
    switch (version) {
    case cluster_version_t::v1_13:
        return vdeserialize<cluster_version_t::v1_13>(s, thing);
    }
}



#endif  // CONTAINERS_ARCHIVE_VERSIONED_HPP_
