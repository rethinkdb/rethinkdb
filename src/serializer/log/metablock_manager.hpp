// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_METABLOCK_MANAGER_HPP_
#define SERIALIZER_LOG_METABLOCK_MANAGER_HPP_

/* Notice:
 * This file defines templatized classes and does not provide their
 * implementations. Those implementations are provided in
 * serializer/log/metablock_manager.tcc which is included by
 * serializer/log/metablock_manager.cc and explicitly instantiates the template
 * on the type log_serializer_metablock_t. If you want to use this type as the
 * template parameter you can do so and it should work fine... if you want to
 * use a different type you may have some difficulty and should look at what's
 * going on in the aforementioned files. */

#include <stddef.h>
#include <string.h>

#include <vector>

#include "errors.hpp"
#include <boost/crc.hpp>

#include "arch/compiler.hpp"
#include "arch/types.hpp"
#include "concurrency/mutex.hpp"
#include "serializer/log/extent_manager.hpp"
#include "serializer/log/static_header.hpp"


#define MB_BLOCKS_PER_EXTENT 8

#define MB_BAD_VERSION (-1)
#define MB_START_VERSION 1



/* TODO support multiple concurrent writes */
static const char MB_MARKER_MAGIC[8] = {'m', 'e', 't', 'a', 'b', 'l', 'c', 'k'};

std::vector<int64_t> initial_metablock_offsets(int64_t extent_size);



template<class metablock_t>
class metablock_manager_t {
public:
    typedef int64_t metablock_version_t;

    // This is stored directly to disk.  Changing it will change the disk format.
    ATTR_PACKED(struct crc_metablock_t {
        char magic_marker[sizeof(MB_MARKER_MAGIC)];
        // The version that differs only when the software is upgraded to a newer
        // version.  This field might allow for in-place upgrading of the cluster.
        uint32_t disk_format_version;
        // The CRC checksum of [disk_format_version]+[version]+[metablock].
        uint32_t _crc;
        // The version that increments every time a metablock is written.
        metablock_version_t version;
        // The value in the metablock (pointing at LBA superblocks, etc).
        metablock_t metablock;
    public:
        void prepare(uint32_t _disk_format_version, metablock_t *mb, metablock_version_t vers) {
            disk_format_version = _disk_format_version;
            metablock = *mb;
            memcpy(magic_marker, MB_MARKER_MAGIC, sizeof(MB_MARKER_MAGIC));
            version = vers;
            _crc = compute_own_crc();
        }
        bool check_crc() {
            return (_crc == compute_own_crc());
        }
    private:
        uint32_t compute_own_crc() {
            boost::crc_32_type crc_computer;
            crc_computer.process_bytes(&disk_format_version, sizeof(disk_format_version));
            crc_computer.process_bytes(&version, sizeof(version));
            crc_computer.process_bytes(&metablock, sizeof(metablock));
            return crc_computer.checksum();
        }
    });

    explicit metablock_manager_t(extent_manager_t *em);
    ~metablock_manager_t();

    /* Clear metablock slots and write an initial metablock to the database file */
    static void create(file_t *dbfile, int64_t extent_size, metablock_t *initial);

    /* Tries to load existing metablocks */
    void co_start_existing(file_t *dbfile, bool *mb_found, metablock_t *mb_out);
    struct metablock_read_callback_t {
        virtual void on_metablock_read() = 0;
        virtual ~metablock_read_callback_t() {}
    };

    bool start_existing(file_t *dbfile, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb);

    struct metablock_write_callback_t {
        virtual void on_metablock_write() = 0;
        virtual ~metablock_write_callback_t() {}
    };
    bool write_metablock(metablock_t *mb, file_account_t *io_account, metablock_write_callback_t *cb);
    void co_write_metablock(metablock_t *mb, file_account_t *io_account);

    void shutdown();

private:
    struct head_t {
    private:
        uint32_t mb_slot;
        uint32_t saved_mb_slot;
    public:
        // whether or not we've wrapped around the edge (used during startup)
        bool wraparound;
    public:
        explicit head_t(metablock_manager_t *mgr);
        metablock_manager_t *const mgr;

        // handles moving along successive mb slots
        void operator++();

        // return the offset we should be writing to
        int64_t offset();

        // save the state to be loaded later (used to save the last known uncorrupted metablock)
        void push();

        //load a previously saved state (stack has max depth one)
        void pop();
    };

    void start_existing_callback(file_t *dbfile, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb);
    void write_metablock_callback(metablock_t *mb, file_account_t *io_account, metablock_write_callback_t *cb);

    mutex_t write_lock;

    // keeps track of where we are in the extents
    head_t head;

    metablock_version_t next_version_number;

    const scoped_device_block_aligned_ptr_t<crc_metablock_t> mb_buffer;
    // true: we're using the buffer, no one else can
    bool mb_buffer_in_use;

    extent_manager_t *const extent_manager;

    const std::vector<int64_t> metablock_offsets;

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
