#ifndef CLUSTER_VERSION_HPP_
#define CLUSTER_VERSION_HPP_

// An enumeration of the current and previous recognized versions.  Every
// serialization / deserialization happens relative to one of these versions.
enum class cluster_version_t {
    // The versions are currently contiguously numbered.  If this becomes untrue,
    // redefine the ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE line below.
    v1_13 = 0,

    // See CLUSTER_VERSION, which should always be the latest version.
    LATEST_VERSION = v1_13,

    // ONLY_VERSION should only exist as long as there's only one version.  A few
    // assertions check it -- maybe that code should be fixed to handle multiple
    // versions, once that's possible.
    ONLY_VERSION = v1_13,
};

#endif  // CLUSTER_VERSION_HPP_
