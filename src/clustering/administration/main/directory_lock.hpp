#ifndef CLUSTERING_ADMINISTRATION_MAIN_DIRECTORY_LOCK_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_DIRECTORY_LOCK_HPP_

#include <unistd.h>

#include <exception>
#include <string>

#include "utils.hpp"
#include "arch/io/io_utils.hpp"

#ifndef _WIN32
#define INVALID_GID (static_cast<gid_t>(-1))
#define INVALID_UID (static_cast<uid_t>(-1))
#endif

bool check_existence(const base_path_t& base_path);
bool check_dir_emptiness(const base_path_t& base_path);

class directory_lock_t {
public:
    // Possibly creates, then opens and locks the specified directory
    // Returns true if the directory was created, false otherwise
    directory_lock_t(const base_path_t &path, bool create, bool *created_out);
    ~directory_lock_t();

    // Prevents deletion of the directory tree at destruction, if
    //  the directory was created in the constructor
    void directory_initialized();

    // Sets the ownership of the directory to the given user and group.
    // INVALID_GID or INVALID_UID is a no-op for that field.
    // The strings are for error messages.
    void change_ownership(gid_t group_id, const std::string &group_name,
                          uid_t user_id, const std::string &user_name);

private:
    const base_path_t directory_path;
    scoped_fd_t directory_fd;
    bool created;
    bool initialize_done;

    DISABLE_COPYING(directory_lock_t);
};

class directory_missing_exc_t : public std::exception {
public:
    explicit directory_missing_exc_t(const base_path_t &path) {
        info = strprintf("The directory '%s' does not exist, run 'rethinkdb create -d \"%s\"' and try again.",
                         path.path().c_str(), path.path().c_str());
    }
    ~directory_missing_exc_t() throw () { }
    const char *what() const throw () {
        return info.c_str();
    }
private:
    std::string info;
};

class directory_create_failed_exc_t : public std::exception {
public:
    directory_create_failed_exc_t(int err, const base_path_t &path) {
        info = strprintf("Could not create directory '%s': %s",
                         path.path().c_str(), errno_string(err).c_str());
    }
    ~directory_create_failed_exc_t() throw () { }
    const char *what() const throw () {
        return info.c_str();
    }
private:
    std::string info;
};

class directory_open_failed_exc_t : public std::exception {
public:
    directory_open_failed_exc_t(int err, const base_path_t &path) {
        info = strprintf("Could not open directory '%s': %s",
                         path.path().c_str(), errno_string(err).c_str());
    }
    ~directory_open_failed_exc_t() throw () { }
    const char *what() const throw () {
        return info.c_str();
    }
private:
    std::string info;
};

class directory_locked_exc_t : public std::exception {
public:
    explicit directory_locked_exc_t(const base_path_t &path) {
        info = strprintf("Directory '%s' is already in use, perhaps another instance of rethinkdb is using it.",
                         path.path().c_str());
    }
    ~directory_locked_exc_t() throw () { }
    const char *what() const throw () {
        return info.c_str();
    }
private:
    std::string info;
};

#endif  // CLUSTERING_ADMINISTRATION_MAIN_DIRECTORY_LOCK_HPP_
