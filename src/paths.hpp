/**
 * @file paths.hpp
 * @brief File system path management for data storage and serialization.
 *
 * Provides classes for managing base directory paths and serializer file paths
 * across different operating systems (Windows, Unix/Linux).
 */

#ifndef PATHS_HPP_
#define PATHS_HPP_

#include <string>

#include "errors.hpp"

/**
 * @defgroup PathManagement Path and File System Management
 * @brief Directory and file path utilities
 */

/**
 * @ingroup PathManagement
 * @def PATH_SEPARATOR
 * @brief Platform-specific path separator.
 *
 * - Windows: "\\\\"
 * - Unix/Linux/macOS: "/"
 */
#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

/**
 * @ingroup PathManagement
 * @brief Manager for RethinkDB's base data directory.
 *
 * Represents the root directory where all RethinkDB data is stored.
 * Can be made absolute (useful for daemonizing).
 *
 * Example:
 * @code
 * base_path_t data_dir("/var/lib/rethinkdb");
 * std::string path = data_dir.path();  // "/var/lib/rethinkdb"
 * @endcode
 */
class base_path_t {
public:
    /**
     * @brief Construct an empty path.
     */
    base_path_t() { }

    /**
     * @brief Construct with a specific directory path.
     *
     * @param path The directory path (can be relative or absolute).
     */
    explicit base_path_t(const std::string& path);

    /**
     * @brief Get the directory path.
     *
     * @return The stored path string.
     */
    const std::string& path() const;

    /**
     * @brief Convert this path to an absolute path.
     *
     * The path must already exist on the filesystem. Useful for daemonization
     * where you need to ensure paths are absolute before changing directories.
     */
    void make_absolute();
private:
    std::string path_;
};

/**
 * @ingroup PathManagement
 * @def TEMPORARY_DIRECTORY_NAME
 * @brief Name of the temporary directory used during serializer file creation.
 *
 * Value: "tmp"
 *
 * Serializer files are created in this temporary directory first, then moved
 * to their permanent location when ready.
 */
static const char *TEMPORARY_DIRECTORY_NAME = "tmp";

class serializer_filepath_t;

namespace unittest {
serializer_filepath_t manual_serializer_filepath(const std::string& permanent_path,
                                                 const std::string& temporary_path);
}  // namespace unittest

/**
 * @ingroup PathManagement
 * @brief Manager for serializer file paths with atomic creation support.
 *
 * Maintains both permanent and temporary paths for a serializer file.
 * Files are created in the temporary location first, then atomically moved
 * to the permanent location once successfully created.
 *
 * Example:
 * @code
 * base_path_t base("/data");
 * serializer_filepath_t file_path(base, "metadata");
 * // Temporary path: /data/tmp/metadata.create
 * // Permanent path: /data/metadata
 * @endcode
 */
class serializer_filepath_t {
public:
    /**
     * @brief Construct file paths from a base directory.
     *
     * Creates temporary and permanent path names based on the base directory
     * and a relative path.
     *
     * @param directory The base directory.
     * @param relative_path The relative path within the base directory.
     */
    serializer_filepath_t(const base_path_t& directory, const std::string& relative_path)
        : permanent_path_(directory.path() + PATH_SEPARATOR + relative_path),
          temporary_path_(directory.path() + PATH_SEPARATOR + TEMPORARY_DIRECTORY_NAME + PATH_SEPARATOR + relative_path + ".create") {
        guarantee(!relative_path.empty());
    }

    /**
     * @brief Get the final permanent path for the file.
     *
     * This is where the file will be moved after successful creation.
     *
     * @return The permanent file path.
     */
    std::string permanent_path() const { return permanent_path_; }

    /**
     * @brief Get the temporary creation path for the file.
     *
     * The file is first created here, then moved to permanent_path()
     * when successfully written.
     *
     * @return The temporary file path.
     */
    std::string temporary_path() const { return temporary_path_; }

private:
    friend serializer_filepath_t unittest::manual_serializer_filepath(const std::string& permanent_path,
                                                                      const std::string& temporary_path);
    serializer_filepath_t(const std::string& _permanent_path, const std::string& _temporary_path)
        : permanent_path_(_permanent_path), temporary_path_(_temporary_path) { }

    const std::string permanent_path_;
    const std::string temporary_path_;
};

/**
 * @ingroup PathManagement
 * @brief Recreate the temporary directory for a base path.
 *
 * Creates or clears the temporary directory used for atomic file creation.
 *
 * @param base_path The base directory.
 */
void recreate_temporary_directory(const base_path_t& base_path);

/**
 * @ingroup PathManagement
 * @brief Recursively remove a directory and all its contents.
 *
 * @param path The path to the directory to remove.
 *
 * @warning Removes all files and subdirectories recursively!
 */
void remove_directory_recursive(const char *path);

/**
 * @ingroup PathManagement
 * @brief Read the entire contents of a file into a string.
 *
 * Blocks until the entire file is read.
 *
 * @param path The file path to read.
 * @return The file contents as a string.
 * @throw Throws exception on read error.
 */
std::string blocking_read_file(const char *path);

/**
 * @ingroup PathManagement
 * @brief Read a file with error handling.
 *
 * @param path The file path to read.
 * @param contents_out Pointer to string to receive the file contents.
 * @return true if successful, false on error.
 */
bool blocking_read_file(const char *path, std::string *contents_out);

#endif  // PATHS_HPP_
