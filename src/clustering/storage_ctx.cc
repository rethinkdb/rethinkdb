#include "clustering/storage_ctx.hpp"

storage_ctx_t::storage_ctx_t(file_direct_io_mode_t direct_io_mode,
                             int max_concurrent_io_requests)
    : backender(direct_io_mode, max_concurrent_io_requests) { }

storage_ctx_t::~storage_ctx_t() { }
