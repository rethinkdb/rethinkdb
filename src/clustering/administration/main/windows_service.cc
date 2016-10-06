// Copyright 2016 RethinkDB, all rights reserved.
#include "clustering/administration/main/windows_service.hpp"

#ifdef _WIN32
#include "arch/runtime/thread_pool.hpp"
#include "arch/types.hpp"

// For some reason the DELETE symbol doesn't get defined properly.
// (it should get defined by Winnt.h which is part of Windows.h)
#define WIN_ACCESS_DELETE (0x00010000L)

DWORD windows_service_control_handler(
        DWORD dw_control,
        DWORD dw_event_type,
        LPVOID,
        LPVOID) {
    switch (dw_control) {
    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    case SERVICE_CONTROL_SHUTDOWN: // fallthru
    case SERVICE_CONTROL_STOP:
        thread_pool_t::interrupt_handler(CTRL_SHUTDOWN_EVENT);
        return NO_ERROR;
    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

void windows_service_main_function(
    const std::function<int(int, char *[])> &main_fun,
    DWORD argc,
    char *argv[]) {
    SERVICE_STATUS_HANDLE status_handle =
        RegisterServiceCtrlHandlerEx("", windows_service_control_handler, nullptr);
    SERVICE_STATUS status;
    memset(&status, 0, sizeof(status));
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = SERVICE_RUNNING;
    status.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
    SetServiceStatus(status_handle, &status);

    int res = main_fun(argc, argv);

    status.dwCurrentState = SERVICE_STOPPED;
    status.dwWin32ExitCode = res == 0 ? 0 : ERROR_SERVICE_SPECIFIC_ERROR;
    status.dwServiceSpecificExitCode = res;
    SetServiceStatus(status_handle, &status);
}

std::string escape_windows_shell_arg(const std::string &arg) {
    std::string res;
    res += "\"";
    for (char ch : arg) {
        if (ch == '\\') {
            res += "\\\\";
        } else if (ch == '"') {
            res += "\\\"";
        } else {
            res += ch;
        }
    }
    res += "\"";
    return res;
}

class service_manager_t {
public:
    explicit service_manager_t(DWORD desired_access) {
        sc_manager = OpenSCManager(
            nullptr /* local computer */,
            nullptr,
            desired_access);
        if (sc_manager == nullptr) {
            DWORD err = GetLastError();
            if (err == ERROR_ACCESS_DENIED) {
                throw windows_privilege_exc_t();
            }
            fprintf(stderr, "Error opening service manager: %s\n", winerr_string(err).c_str());
        }
    }
    ~service_manager_t() {
        if (sc_manager != nullptr) {
            CloseServiceHandle(sc_manager);
        }
    }
    SC_HANDLE get() {
        return sc_manager;
    }
private:
    SC_HANDLE sc_manager;
};

class service_handle_t {
public:
    explicit service_handle_t(SC_HANDLE existing_handle) {
        service = existing_handle;
    }
    service_handle_t(
            service_manager_t *sc_manager,
            const std::string &service_name,
            DWORD desired_access) {
        service = OpenService(
            sc_manager->get(),
            service_name.c_str(),
            desired_access);
        if (service == nullptr) {
            DWORD err = GetLastError();
            if (err == ERROR_ACCESS_DENIED) {
                throw windows_privilege_exc_t();
            }
            fprintf(stderr, "Opening the service `%s` failed: %s\n",
                service_name.c_str(), winerr_string(err).c_str());
        }
    }
    ~service_handle_t() {
        if (service != nullptr) {
            CloseServiceHandle(service);
        }
    }
    SC_HANDLE get() {
        return service;
    }
private:
    SC_HANDLE service;
};

bool install_windows_service(
    const std::string &service_name,
    const std::string &display_name,
    const std::string &bin_path,
    const std::vector<std::string> &arguments,
    const char *account_name,
    const char *account_password) {

    // The following code is loosely based on the example at
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms683500(v=vs.85).aspx

    // Get a handle to the SCM database. 
    service_manager_t sc_manager(SC_MANAGER_CREATE_SERVICE);
    if (sc_manager.get() == nullptr) {
        return false;
    }

    // Build the service path (escaped binary path + arguments)
    std::string service_path = escape_windows_shell_arg(bin_path);
    for (const auto &arg : arguments) {
        service_path += " ";
        service_path += escape_windows_shell_arg(arg);
    }

    // Create the service
    service_handle_t service(CreateService(
        sc_manager.get(),
        service_name.c_str(),
        display_name.c_str(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START /* start the service on system startup */,
        SERVICE_ERROR_NORMAL,
        service_path.c_str(),
        nullptr /* no load ordering group  */,
        nullptr /* no tag identifier */,
        nullptr /* no dependencies */,
        account_name,
        account_password));

    if (service.get() == nullptr) {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED) {
            throw windows_privilege_exc_t();
        }
        fprintf(stderr, "Creating the service `%s` failed: %s\n",
                service_name.c_str(), winerr_string(err).c_str());
        return false;
    }

    // Set the service description
    SERVICE_DESCRIPTION description;
    std::string description_str = "RethinkDB database server (" + service_name + ").";
    description.lpDescription = const_cast<LPTSTR>(description_str.c_str());
    if (!ChangeServiceConfig2(service.get(), SERVICE_CONFIG_DESCRIPTION, &description)) {
        DWORD err = GetLastError();
        fprintf(stderr, "Failed to change the service description for service `%s`: %s\n",
                service_name.c_str(), winerr_string(err).c_str());
        return false;
    } else {
        return true;
    }
}

bool remove_windows_service(const std::string &service_name) {

    // Get a handle to the SCM database. 
    service_manager_t sc_manager(SC_MANAGER_CONNECT);
    if (sc_manager.get() == nullptr) {
        return false;
    }

    // Open the service
    service_handle_t service(&sc_manager, service_name, WIN_ACCESS_DELETE);
    if (service.get() == nullptr) {
        return false;
    }

    // Delete the service
    if (!DeleteService(service.get())) {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED) {
            throw windows_privilege_exc_t();
        }
        fprintf(stderr, "Deleting the service `%s` failed: %s\n",
                service_name.c_str(), winerr_string(err).c_str());
        return false;
    } else {
        return true;
    }
}

bool start_windows_service(const std::string &service_name) {
    // Get a handle to the SCM database. 
    service_manager_t sc_manager(SC_MANAGER_CONNECT);
    if (sc_manager.get() == nullptr) {
        return false;
    }

    // Open the service
    service_handle_t service(&sc_manager, service_name, SERVICE_START);
    if (service.get() == nullptr) {
        return false;
    }

    // Start the service
    if (!StartService(service.get(), 0, nullptr)) {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED) {
            throw windows_privilege_exc_t();
        } else if (err == ERROR_SERVICE_ALREADY_RUNNING) {
            fprintf(stderr, "The service `%s` is already running.\n",
                    service_name.c_str());
            return true;
        }
        fprintf(stderr, "Starting the service `%s` failed: %s\n",
            service_name.c_str(), winerr_string(err).c_str());
        return false;
    } else {
        return true;
    }
}

bool stop_windows_service(const std::string &service_name) {
    // Get a handle to the SCM database. 
    service_manager_t sc_manager(SC_MANAGER_CONNECT);
    if (sc_manager.get() == nullptr) {
        return false;
    }

    // Open the service
    service_handle_t service(&sc_manager, service_name, SERVICE_STOP);
    if (service.get() == nullptr) {
        return false;
    }

    // Stop the service
    SERVICE_STATUS last_status;
    if (!ControlService(service.get(), SERVICE_CONTROL_STOP, &last_status)) {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED) {
            throw windows_privilege_exc_t();
        } else if (err == ERROR_SERVICE_NOT_ACTIVE) {
            fprintf(stderr, "The service `%s` is already stopped.\n",
                service_name.c_str());
            return true;
        }
        fprintf(stderr, "Stopping the service `%s` failed: %s\n",
            service_name.c_str(), winerr_string(err).c_str());
        return false;
    } else {
        return true;
    }
}

#endif /* _WIN32 */
