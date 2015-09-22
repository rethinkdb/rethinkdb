// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/arch.hpp"

#include "arch/runtime/coroutines.hpp"
#include "logger.hpp"
#include "containers/scoped.hpp"

struct io_coroutine_adapter_t : public iocallback_t {
    coro_t *cont;
    io_coroutine_adapter_t() : cont(coro_t::self()) { }
    void on_io_complete() {
        cont->notify_later_ordered();
    }

#ifdef _WIN32
    // TODO ATN: this helps debugging but is inelegant
    HANDLE handle = INVALID_HANDLE_VALUE;
    io_coroutine_adapter_t(HANDLE handle_) : cont(coro_t::self()), handle(handle_) { }
    void on_io_failure(int errsv, int64_t offset, int64_t count) {
        if (handle != INVALID_HANDLE_VALUE) {
            /*
            const int name_info_size = MAX_PATH + sizeof(FILE_NAME_INFO);
            scoped_malloc_t<FILE_NAME_INFO> name_info(name_info_size);
            BOOL res = GetFileInformationByHandleEx(handle, FileNameInfo, static_cast<void*>(name_info.get()), name_info_size);
            if (res) {
                logERR("read/write error in file '%s'", name_info->FileName); // TODO ATN: the reported file name seems wrong
            }
            */
            char path [MAX_PATH];
            DWORD res = GetFinalPathNameByHandle(handle, path, sizeof(path), FILE_NAME_OPENED);
            if (res == NO_ERROR) {
                logERR("read/write error in file '%s'", path);
            }
        }
        iocallback_t::on_io_failure(errsv, offset, count);
    }
#endif
};

void co_read(file_t *file, int64_t offset, size_t length, void *buf, file_account_t *account) {
#ifdef _WIN32
    io_coroutine_adapter_t adapter(file->maybe_get_fd());
#else
    io_coroutine_adapter_t adapter;
#endif
    file->read_async(offset, length, buf, account, &adapter);
    coro_t::wait();
}

void co_write(file_t *file, int64_t offset, size_t length, void *buf,
              file_account_t *account, file_t::wrap_in_datasyncs_t wrap_in_datasyncs) {
#ifdef _WIN32
    io_coroutine_adapter_t adapter(file->maybe_get_fd());
#else
    io_coroutine_adapter_t adapter;
#endif
    file->write_async(offset, length, buf, account, &adapter, wrap_in_datasyncs);
    coro_t::wait();
}
