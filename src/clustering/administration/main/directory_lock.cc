#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>

#include "clustering/administration/main/directory_lock.hpp"

bool check_existence(const base_path_t& base_path) {
    return 0 == access(base_path.path().c_str(), F_OK);
}

directory_lock_t::directory_lock_t(const base_path_t &path, bool create, bool *is_new_directory) :
    directory_path(path),
    created(false),
    initialize_done(false) {

    *is_new_directory = false;
    bool dir_exists = check_existence(directory_path);

    if (!dir_exists) {
        if (!create) {
            throw directory_missing_exc_t(directory_path);
        }
        int mkdir_res = mkdir(directory_path.path().c_str(), 0755);
        if (mkdir_res != 0) {
            throw directory_create_failed_exc_t(errno, directory_path);
        }
        created = true;
        *is_new_directory = true;
    } else if (create) {
        serializer_filepath_t serializer_path = serializer_filepath_t(path, "metadata");
        base_path_t metadata_path = base_path_t(serializer_path.permanent_path());
        bool metadata_exists = check_existence(metadata_path);

        if (!metadata_exists) {
            *is_new_directory = true;
        }
     }

    directory_fd.reset(::open(directory_path.path().c_str(), O_RDONLY));
    if (directory_fd.get() == INVALID_FD) {
        throw directory_open_failed_exc_t(errno, directory_path);
    }

    if (flock(directory_fd.get(), LOCK_EX | LOCK_NB) != 0) {
        throw directory_locked_exc_t(directory_path);
    }
}

directory_lock_t::~directory_lock_t() {
    // Only delete the directory if we created it and haven't finished initialization
    if (created && !initialize_done) {
        remove_directory_recursive(directory_path.path().c_str());
    }
}

void directory_lock_t::directory_initialized() {
    guarantee(directory_fd.get() != INVALID_FD);
    initialize_done = true;
}

