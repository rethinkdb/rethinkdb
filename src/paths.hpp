/// @file paths.hpp
/// @brief File path management and manipulation utilities
///
/// Provides abstractions for managing RethinkDB data directory paths and
/// serializer file paths. Handles temporary file creation and atomic moves
/// to permanent locations.
///
/// @defgroup PathManagement Path Management Utilities
/// File path abstractions for database storage
/// @{

#ifndef PATHS_HPP_
#define PATHS_HPP_

#include <string>

#include "errors.hpp"

/// @brief Platform-specific path separator
/// @details Defined as "\\" on Windows, "/" on POSIX systems
#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

/// @brief Encapsulates the base directory for all RethinkDB data storage
///
/// Represents the root directory where all database files and data are stored.
/// Can be made absolute for proper daemonization support.
///
/// @example
/// @code
/// base_path_t base_path("/var/lib/rethinkdb/default");
/// // Use base_path to construct file paths
/// @endcode
class base_path_t {
public:
    /// @brief Constructs an empty (uninitialized) base path
    base_path_t() { }

    /// @brief Constructs a base path from a string path
    /// @param path The directory path as a string
    explicit base_path_t(const std::string& path);

    /// @brief Returns the path as a constant string reference
    /// @return The directory path string
    const std::string& path() const;

    /// @brief Converts a relative path to an absolute path
    /// Resolves the path relative to the current working directory.
    /// The path must exist before calling this function.
    /// Useful for daemonization to ensure paths remain valid
    /// after changing the working directory.
    /// @note Path must exist before calling this function
    void make_absolute();

private:
    /// @internal The stored path string
    std::string path_;
};

/// @brief Standard name for temporary directory within the data directory
static const char *TEMPORARY_DIRECTORY_NAME = "tmp";

class serializer_filepath_t;

/// @brief Unit test utilities for filepath manipulation
namespace unittest {
/// @brief Creates a serializer filepath with explicit permanent and temporary paths
/// Used for unit testing to bypass normal path construction
/// @param permanent_path The final path for the completed file
/// @param temporary_path The temporary path during file creation
/// @return A serializer_filepath_t with the specified paths
serializer_filepath_t manual_serializer_filepath(const std::string& permanent_path,
                                                 const std::string& temporary_path);
}  // namespace unittest

/// @brief Encapsulates paths for a serializer data file
///
/// Manages both the permanent location of a serializer file and its
/// temporary location during creation. Supports atomic file creation
/// by writing to temporary location then moving to permanent location.
///
/// @example
/// @code
/// base_path_t base("/var/lib/rethinkdb");
/// serializer_filepath_t file(base, "data/index.db");
/// // Write to file.temporary_path() during creation
/// // Move to file.permanent_path() when complete
/// @endcode
class serializer_filepath_t {
public:
    /// @brief Constructs a serializer filepath from base directory and relative path
    /// Creates permanent path as base + "/" + relative_path
    /// and temporary path as base + "/tmp/" + relative_path + ".create"
    /// @param directory The base data directory
    /// @param relative_path The relative path from base (must not be empty)
    serializer_filepath_t(const base_path_t& directory, const std::string& relative_path)
        : permanent_path_(directory.path() + PATH_SEPARATOR + relative_path),
          temporary_path_(directory.path() + PATH_SEPARATOR + TEMPORARY_DIRECTORY_NAME + PATH_SEPARATOR + relative_path + ".create") {
        guarantee(!relative_path.empty());
    }

    /// @brief Returns the final permanent location of the file
    /// This is the target location after the file is successfully created
    /// @return The permanent file path
    std::string permanent_path() const { return permanent_path_; }

    /// @brief Returns the temporary location for file creation
    /// Files should be written to this location first, then moved to permanent_path()
    /// @return The temporary file path
    std::string temporary_path() const { return temporary_path_; }

private:
    /// @internal Unit test friend access for manual_serializer_filepath
    friend serializer_filepath_t unittest::manual_serializer_filepath(const std::string& permanent_path,
                                                                      const std::string& temporary_path);

    /// @internal Private constructor for unit tests
    serializer_filepath_t(const std::string& _permanent_path, const std::string& _temporary_path)
        : permanent_path_(_permanent_path), temporary_path_(_temporary_path) { }

    /// @internal The permanent file path
    const std::string permanent_path_;
    /// @internal The temporary file path
    const std::string temporary_path_;
};

/// @brief Recreates the temporary directory structure in the given base path
/// Ensures the temporary directory exists and is ready for file creation
/// @param base_path The base directory containing the temporary folder
void recreate_temporary_directory(const base_path_t& base_path);

/// @brief Recursively removes a directory and all its contents
/// Deletes the directory tree rooted at @p path and all files within
/// @param path The directory path to remove (C-string)
/// @warning This operation is recursive and permanent
void remove_directory_recursive(const char *path);

/// @brief Reads an entire file into a string (blocking operation)
/// Opens, reads, and closes the file in one operation.
/// Crashes if the file cannot be read.
/// @param path The file path (C-string)
/// @return The complete file contents as a string
/// @example
/// @code
/// std::string data = blocking_read_file("/path/to/config.json");
/// @endcode
std::string blocking_read_file(const char *path);

/// @brief Reads an entire file into a string with error reporting
/// Opens, reads, and closes the file, returning success/failure status
/// @param path The file path (C-string)
/// @param contents_out Pointer to string to store contents (on success)
/// @return true if file was read successfully, false on error
/// @example
/// @code
/// std::string contents;
/// if (blocking_read_file("/path/to/file", &contents)) {
///     // Process contents
/// } else {
///     // Handle read error
/// }
/// @endcode
bool blocking_read_file(const char *path, std::string *contents_out);

/// @}

#endif  // PATHS_HPP_
