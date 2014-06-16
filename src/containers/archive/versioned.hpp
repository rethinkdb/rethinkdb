#ifndef CONTAINERS_ARCHIVE_VERSIONED_HPP_
#define CONTAINERS_ARCHIVE_VERSIONED_HPP_

#include "containers/archive/archive.hpp"
#include "version.hpp"

// This defines how to serialize cluster_version_t (which conveniently has a
// contiguous set of valid representation, from v1_13 to v1_13).
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(cluster_version_t, int8_t,
                                      cluster_version_t::v1_13,
                                      cluster_version_t::ONLY_VERSION);

// Serializes a value for a given version.  DOES NOT SERIALIZE THE VERSION NUMBER!
template <class T>
void serialize_for_version(cluster_version_t version, write_message_t *wm,
                           const T &value) {
    rassert(version == cluster_version_t::ONLY_VERSION);
    switch (version) {
    case cluster_version_t::v1_13:
        serialize<cluster_version_t::v1_13>(wm, value);
    }
}

// Deserializes a value, assuming it's serialized for a given version.  (This doesn't
// deserialize any version numbers.)
template <class T>
archive_result_t deserialize_for_version(cluster_version_t version,
                                         read_stream_t *s,
                                         T *thing) {
    rassert(version == cluster_version_t::ONLY_VERSION);
    switch (version) {
    case cluster_version_t::v1_13:
        return deserialize<cluster_version_t::v1_13>(s, thing);
    }
}


// RSI: Remove this, or something.
struct bogus_made_up_struct_t;
template <cluster_version_t W>
size_t serialized_size(const bogus_made_up_struct_t &);

// RSI: This is completely unused right now.  (Should it be?)
template <class T>
size_t serialized_size_for_version(cluster_version_t version,
                                   const T &thing) {
    rassert(version == cluster_version_t::ONLY_VERSION);
    switch (version) {
    case cluster_version_t::v1_13:
        return serialized_size<cluster_version_t::v1_13>(thing);
    }
}

// RSI: use v1_13_is_latest_version or something, idk.
#define INSTANTIATE_SERIALIZE_SINCE_v1_13(typ)          \
    template void serialize<cluster_version_t::v1_13>(  \
            write_message_t *, const typ &)

// RSI: use v1_13_is_latest_version or something, idk.
#define INSTANTIATE_DESERIALIZE_SINCE_v1_13(typ)                        \
    template archive_result_t deserialize<cluster_version_t::v1_13>(    \
            read_stream_t *, typ *)

// RSI: use v1_13_is_latest_version or something, idk.
#define INSTANTIATE_SERIALIZED_SIZE_SINCE_v1_13(typ) \
    template size_t serialized_size<cluster_version_t::v1_13>(const typ &)

// RSI: Probably rename this?
// RSI: use v1_13_is_latest_version or something, idk.
#define INSTANTIATE_SINCE_v1_13(typ)            \
    INSTANTIATE_SERIALIZE_SINCE_v1_13(typ);     \
    INSTANTIATE_DESERIALIZE_SINCE_v1_13(typ)

// RSI: use v1_13_is_latest_version or something, idk.
#define INSTANTIATE_SELF_SINCE_v1_13(typ)                               \
    template void typ::rdb_serialize<cluster_version_t::v1_13>(         \
            write_message_t *) const;                                   \
    template archive_result_t typ::rdb_deserialize<cluster_version_t::v1_13>( \
            read_stream_t *s)


#endif  // CONTAINERS_ARCHIVE_VERSIONED_HPP_
