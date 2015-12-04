#ifndef ARCH_FILESYSTEM_HPP_
#define ARCH_FILESYSTEM_HPP_

#ifndef _WIN32
#include <libgen.h>
#endif

#include <string>

#include "errors.hpp"

std::string strdirname(const std::string path) {
#ifdef _WIN32
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    errno_t err = _splitpath_s(path.c_str(), drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
    guarantee_xerr(!err, err, "_splitpath_s failed");
    char ret[_MAX_PATH];
    err = _makepath_s(ret, _MAX_PATH, drive, dir, NULL, NULL);
    guarantee_xerr(!err, err, "_makepath_s failed");
    return ret;
#else
    std::vector<char> path_copy(path.begin(), path.end());
    path_copy.push_back(0);
    return ::dirname(path_copy.data());
#endif
}

#endif
