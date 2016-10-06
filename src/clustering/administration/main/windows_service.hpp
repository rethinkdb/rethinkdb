// Copyright 2016 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_WINDOWS_SERVICE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_WINDOWS_SERVICE_HPP_
#ifdef _WIN32

#include <exception>
#include <functional>
#include <string>
#include <vector>

#include "errors.hpp"

#include <Windows.h>


class windows_privilege_exc_t : public std::exception {
};

// Helper function to escape a path or argument
// For example
//   c:\test folder\test.exe
// gets turned into
//   "c:\\test folder\\test.exe"
std::string escape_windows_shell_arg(const std::string &arg);

void windows_service_main_function(
    const std::function<int(int, char *[])> &main_fun,
    DWORD argc,
    char *argv[]);

bool install_windows_service(
    const std::string &service_name,
    const std::string &display_name,
    const std::string &bin_path,
    const std::vector<std::string> &arguments,
    const char *account_name = nullptr /* LocalSystem account if nullptr */,
    const char *account_password = nullptr);
bool remove_windows_service(const std::string &service_name);

bool start_windows_service(const std::string &service_name);
bool stop_windows_service(const std::string &service_name);

#endif /* _WIN32 */
#endif /* CLUSTERING_ADMINISTRATION_MAIN_WINDOWS_SERVICE_HPP_ */
