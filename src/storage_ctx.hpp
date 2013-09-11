#ifndef CLUSTERING_STORAGE_CTX_HPP_
#define CLUSTERING_STORAGE_CTX_HPP_

#include "arch/io/disk.hpp"
#include "buffer_cache/global_page_repl.hpp"

class storage_ctx_t {
public:
    storage_ctx_t(file_direct_io_mode_t direct_io_mode,
                  int max_concurrent_io_requests = DEFAULT_MAX_CONCURRENT_IO_REQUESTS);
    ~storage_ctx_t();

    io_backender_t io_backender;
    global_page_repl_t global_page_repl;
};

#endif  // CLUSTERING_STORAGE_CTX_HPP_
