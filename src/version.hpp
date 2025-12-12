#ifndef VERSION_HPP_
#define VERSION_HPP_

/// @file version.hpp
/// @brief RethinkDB version enumerations and version management
///
/// Defines cluster version and serialization version enumerations to manage
/// backward compatibility with different RethinkDB versions. Every serialization
/// and deserialization operation is relative to a specific version.
///
/// @defgroup VersionManagement Version Management
/// Version tracking for cluster compatibility and serialization
/// @{

/// @brief Enumeration of obsolete cluster versions (no longer supported)
///
/// Represents RethinkDB versions that are no longer actively supported but
/// may still be referenced in legacy code or during version migrations.
///
/// @example
/// @code
/// if (old_version == obsolete_cluster_version_t::v1_13) {
///     // Handle migration from RethinkDB 1.13
/// }
/// @endcode
enum class obsolete_cluster_version_t {
    v1_13 = 0,              ///< RethinkDB 1.13
    v1_13_2 = 1,            ///< RethinkDB 1.13.2
    v1_13_2_is_latest = v1_13_2  ///< Latest obsolete version
};

/// @brief Enumeration of current and supported cluster versions
///
/// Represents all supported RethinkDB cluster versions. These versions are used
/// to determine compatibility for cluster communication and data serialization.
/// Versions are contiguously numbered for simplified serialization.
///
/// @note When adding a new version, update LATEST_OVERALL and LATEST_DISK appropriately
///
/// @example
/// @code
/// if (current_version >= cluster_version_t::v2_0) {
///     // Use v2.0+ features
/// }
/// @endcode
enum class cluster_version_t {
    /// @defgroup ClusterVersions Supported Cluster Versions
    /// @{
    
    v1_14 = 2,            ///< RethinkDB 1.14
    v1_15 = 3,            ///< RethinkDB 1.15
    v1_16 = 4,            ///< RethinkDB 1.16
    v2_0 = 5,             ///< RethinkDB 2.0
    v2_1 = 6,             ///< RethinkDB 2.1
    v2_2 = 7,             ///< RethinkDB 2.2
    v2_3 = 8,             ///< RethinkDB 2.3
    v2_4 = 9,             ///< RethinkDB 2.4

    /// @brief Latest cluster version (updated with each release)
    /// Used in template instantiations and version checks. Must be updated
    /// when a new cluster version is created.
    v2_4_is_latest = v2_4,

    /// @brief Latest disk serialization version
    /// Used for on-disk data format. Can differ from LATEST_OVERALL for
    /// backward-compatible features. Also represents the latest version of
    /// ReQL deterministic function behavior.
    v2_4_is_latest_disk = v2_4,

    /// @brief Overall latest version (max of cluster and disk versions)
    /// Represents the newest RethinkDB version the system knows about.
    LATEST_OVERALL = v2_4_is_latest,

    /// @brief Latest disk serialization version
    /// This can sometimes differ from the cluster version to maintain
    /// compatibility with older on-disk formats. Also defines the latest
    /// version of ReQL deterministic function behavior.
    LATEST_DISK = v2_4,
    
    /// @}


    // This exists as long as the clustering code only supports the use of one
    // version.  It uses cluster_version_t::CLUSTER wherever it uses this.
    CLUSTER = LATEST_OVERALL,
};

// Uncomment this if cluster_version_t::LATEST_DISK != cluster_version_t::CLUSTER.
// Comment it otherwise. This macro is used to avoid instantiating the same version
// twice in the `INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK` macro.
#define CLUSTER_AND_DISK_VERSIONS_ARE_SAME

#ifdef CLUSTER_AND_DISK_VERSIONS_ARE_SAME
static_assert(cluster_version_t::CLUSTER == cluster_version_t::LATEST_DISK,
              "Comment #define CLUSTER_AND_DISK_VERSIONS_ARE_SAME in version.hpp");
#else
static_assert(cluster_version_t::CLUSTER != cluster_version_t::LATEST_DISK,
              "Uncomment #define CLUSTER_AND_DISK_VERSIONS_ARE_SAME in version.hpp");
#endif

// We will not (barring a bug) even attempt to deserialize a version number that we
// do not support.  Cluster nodes connecting to us make sure that they are
// communicating with a version number that we also support (the max of our two
// versions) and we don't end up deserializing version numbers in that situation
// anyway (yet).  Files on disk will not (barring corruption, or a bug) cause us to
// try to use a version number that we do not support -- we'll see manually whether
// we can read the serializer file by looking at the disk_format_version field in the
// metablock.
//
// At some point the set of cluster versions and disk versions that we support might
// diverge.  It's likely that we'd support a larger window of serialization versions
// in the on-disk format.
//
// Also, note: it's possible that versions will not be linearly ordered: Suppose we
// release v1.17 and then v1.18.  Perhaps v1.17 supports v1_16 and v1_17 and v1.18
// supports v1_17 and v1_18.  Suppose we then need to make a point-release, v1.17.1,
// that changes a serialization format, and we add a new cluster version v1_17_1.
// Then, v1.18 would still only support v1_17 and v1_18.  However, v1.18.1 might
// support v1_17, v1_17_1, and v1_18 (and v1_18_1 if that needs to be created).


enum class obsolete_reql_version_t {
    v1_13 = 0,
    v1_14 = 1,
    v1_15 = v1_14,

    v1_15_is_latest = v1_15,

    EARLIEST = v1_13,
    LATEST = v1_15_is_latest
};

// Reql versions define how secondary index functions should be evaluated.  Older
// versions have bugs that are fixed in newer versions.  They also define how secondary
// index keys are generated.
enum class reql_version_t {
    v1_16 = 2,
    v2_0 = 3,
    v2_1 = 4,
    v2_2 = 5,
    v2_3 = 6,
    v2_4 = 7,

    // Code that uses _is_latest may need to be updated when the
    // version changes
    v2_4_is_latest = v2_4,

    EARLIEST = v1_16,
    LATEST = v2_4_is_latest
};

// Serialization of reql_version_t is defined in protocol_api.hpp.

#endif  // VERSION_HPP_
