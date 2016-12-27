// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_LBA_EXTENT_HPP_
#define SERIALIZER_LOG_LBA_EXTENT_HPP_

#include <vector>

#include "containers/optional.hpp"
#include "arch/types.hpp"
#include "serializer/log/extent_manager.hpp"
#include "serializer/log/types.hpp"

struct extent_block_t;

class extent_t {
    friend struct extent_block_t;

public:
    // Creates new extent
    extent_t(extent_manager_t *em, file_t *file);
    // Recreates extent at given offset (used during startup)
    extent_t(extent_manager_t *em, file_t *file, int64_t loc, size_t size);

    // Releases extent and destroys structure in memory
    void destroy(extent_transaction_t *txn);
    // Only destroys structure in memory
    void shutdown();

public:
    struct read_callback_t : private iocallback_t {
        friend class extent_t;
        virtual void on_extent_read() = 0;
    private:
        void on_io_complete() { on_extent_read(); }
    };
    void read(size_t pos, size_t length, void *buffer, read_callback_t *);

    void append(void *buffer, size_t length, file_account_t *io_account,
                optional<std::vector<checksum_filerange>> *checksums);

    struct completion_callback_t {
        virtual void on_extent_completion() = 0;
        virtual ~completion_callback_t() {}
    };
    void wait_for_write_completion(completion_callback_t *cb);

    extent_reference_t extent_ref;
    size_t amount_filled;

private:
    ~extent_t();   // Use destroy() or shutdown() instead
    extent_manager_t *const em;
    file_t *file;
    extent_block_t *last_block, *current_block;

    DISABLE_COPYING(extent_t);
};

#endif /* SERIALIZER_LOG_LBA_EXTENT_HPP_ */
