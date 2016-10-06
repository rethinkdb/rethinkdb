#ifndef PATHS_HPP_
#define PATHS_HPP_

#include <string>

#include "errors.hpp"

#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

// Contains the name of the directory in which all data is stored.
class base_path_t {
public:
    // Constructs an empty path.
    base_path_t() { }
    explicit base_path_t(const std::string& path);
    const std::string& path() const;

    // Make this base_path_t into an absolute path (useful for daemonizing)
    // This can only be done if the path already exists, which is why we don't do it at construction
    void make_absolute();
private:
    std::string path_;
};

static const char *TEMPORARY_DIRECTORY_NAME = "tmp";

class serializer_filepath_t;

namespace unittest {
serializer_filepath_t manual_serializer_filepath(const std::string& permanent_path,
                                                 const std::string& temporary_path);
}  // namespace unittest

// Contains the name of a serializer file.
class serializer_filepath_t {
public:
    serializer_filepath_t(const base_path_t& directory, const std::string& relative_path)
        : permanent_path_(directory.path() + PATH_SEPARATOR + relative_path),
          temporary_path_(directory.path() + PATH_SEPARATOR + TEMPORARY_DIRECTORY_NAME + PATH_SEPARATOR + relative_path + ".create") {
        guarantee(!relative_path.empty());
    }

    // A serializer_file_opener_t will first open the file in a temporary location, then move it to
    // the permanent location when it's finished being created.  These give the names of those
    // locations.
    std::string permanent_path() const { return permanent_path_; }
    std::string temporary_path() const { return temporary_path_; }

private:
    friend serializer_filepath_t unittest::manual_serializer_filepath(const std::string& permanent_path,
                                                                      const std::string& temporary_path);
    serializer_filepath_t(const std::string& _permanent_path, const std::string& _temporary_path)
        : permanent_path_(_permanent_path), temporary_path_(_temporary_path) { }

    const std::string permanent_path_;
    const std::string temporary_path_;
};

void recreate_temporary_directory(const base_path_t& base_path);

void remove_directory_recursive(const char *path);

std::string blocking_read_file(const char *path);
bool blocking_read_file(const char *path, std::string *contents_out);

#endif  // PATHS_HPP_
