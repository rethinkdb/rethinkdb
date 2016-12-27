// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_METABLOCK_MANAGER_HPP_
#define SERIALIZER_LOG_METABLOCK_MANAGER_HPP_

#include <stddef.h>
#include <string.h>

#include <vector>

#include "arch/compiler.hpp"
#include "arch/types.hpp"
#include "concurrency/mutex.hpp"
#include "serializer/log/extent_manager.hpp"
#include "serializer/log/static_header.hpp"
#include "serializer/log/metablock.hpp"


#define MB_BLOCKS_PER_EXTENT 8

#define MB_BAD_VERSION (-1)
#define MB_START_VERSION 1



/* TODO support multiple concurrent writes */

std::vector<int64_t> initial_metablock_offsets(int64_t extent_size);

class metablock_manager_t {
public:
    explicit metablock_manager_t(extent_manager_t *em);
    ~metablock_manager_t();

    /* Clear metablock slots and write an initial metablock to the database file.
       'initial' has its log_serializer_metablock_t part pre-filled, the rest
       zero-wiped. */
    static void create(file_t *dbfile, int64_t extent_size,
                       scoped_device_block_aligned_ptr_t<crc_metablock_t> &&initial);

    /* Tries to load existing metablocks */
    void co_start_existing(file_t *dbfile, bool *mb_found,
                           log_serializer_metablock_t *mb_out);
    struct metablock_read_callback_t {
        virtual void on_metablock_read() = 0;
        virtual ~metablock_read_callback_t() {}
    };

    bool start_existing(file_t *dbfile, bool *mb_found,
                        log_serializer_metablock_t *mb_out,
                        metablock_read_callback_t *cb);

    struct metablock_write_callback_t {
        virtual void on_metablock_write() = 0;
        virtual ~metablock_write_callback_t() {}
    };
    // crc_mb->metablock must be initialized, the rest zeroed, DEVICE_BLOCK_SIZE-aligned.
    void write_metablock(const scoped_device_block_aligned_ptr_t<crc_metablock_t> &crc_mb,
                         file_account_t *io_account,
                         optional<std::vector<checksum_filerange>> &&checksums,
                         metablock_write_callback_t *cb);
    void co_write_metablock(const scoped_device_block_aligned_ptr_t<crc_metablock_t> &mb,
                            file_account_t *io_account,
                            optional<std::vector<checksum_filerange>> &&checksums);

    void shutdown();

private:
    void start_existing_callback(file_t *dbfile,
                                 bool *mb_found,
                                 log_serializer_metablock_t *mb_out,
                                 metablock_read_callback_t *cb);
    void write_metablock_callback(
            const scoped_device_block_aligned_ptr_t<crc_metablock_t> *mb,
            file_account_t *io_account,
            optional<std::vector<checksum_filerange>> *checksums,
            metablock_write_callback_t *cb);

    bool verify_checksum_fileranges(const crc_metablock_t *mb);

    // Only one metablock write can happen at a time.  This isn't necessarily a
    // desirable thing, but that's how this type works.
    mutex_t write_lock;

    // Which metablock slot we should write next.
    size_t next_mb_slot;

    metablock_version_t next_version_number;

    extent_manager_t *const extent_manager;

    enum state_t {
        state_unstarted,
        state_reading,
        state_ready,
        state_writing,
        state_shut_down
    } state;

    file_t *dbfile;

    DISABLE_COPYING(metablock_manager_t);
};

#endif /* SERIALIZER_LOG_METABLOCK_MANAGER_HPP_ */
