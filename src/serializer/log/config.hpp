#ifndef SERIALIZER_LOG_CONFIG_HPP_
#define SERIALIZER_LOG_CONFIG_HPP_

#include "config/args.hpp"
#include "serializer/types.hpp"
#include "arch/io/io_utils.hpp"   /* for `io_backend_t` */
#include "rpc/serialize_macros.hpp"

/* Private serializer dynamic configuration values */

struct log_serializer_private_dynamic_config_t {
    std::string db_filename;

    // TODO: Get rid of this field, instead have a method that
    // retrieves the value based on db_filename.
#ifdef SEMANTIC_SERIALIZER_CHECK
    std::string semantic_filename;
#endif

    log_serializer_private_dynamic_config_t() { }

    explicit log_serializer_private_dynamic_config_t(const std::string& _db_filename)
        : db_filename(_db_filename)
#ifdef SEMANTIC_SERIALIZER_CHECK
          , semantic_filename(db_filename + "_semantic")
#endif
    { }

#ifdef SEMANTIC_SERIALIZER_CHECK
    RDB_MAKE_ME_SERIALIZABLE_2(db_filename, semantic_filename);
#else
    RDB_MAKE_ME_SERIALIZABLE_1(db_filename);
#endif
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(io_backend_t, int8_t, AIO_BACKEND_MIN_BOUND, AIO_BACKEND_MAX_BOUND);

/* Configuration for the serializer that can change from run to run */

struct log_serializer_dynamic_config_t {
    log_serializer_dynamic_config_t() {
        gc_low_ratio = DEFAULT_GC_LOW_RATIO;
        gc_high_ratio = DEFAULT_GC_HIGH_RATIO;
        num_active_data_extents = DEFAULT_ACTIVE_DATA_EXTENTS;
        file_size = 0;   // Unlimited file size
        file_zone_size = GIGABYTE;
        read_ahead = true;
        io_backend = aio_pool;
        io_batch_factor = DEFAULT_IO_BATCH_FACTOR;
    }

    /* When the proportion of garbage blocks hits gc_high_ratio, then the serializer will collect
    garbage until it reaches gc_low_ratio. */
    float gc_low_ratio, gc_high_ratio;

    /* How many data block extents the serializer will be writing to at once */
    unsigned num_active_data_extents;

    /* If file_size is nonzero and the serializer is not running on a block device, then it will
    pretend to be running on a block device by immediately resizing the file to file_size and then
    zoning it like a block device. */
    size_t file_size;

    /* How big to make each zone if the database is on a block device or if file_size is given */
    size_t file_zone_size;

    /* Which i/o backend should the log serializer use for accessing files? */
    io_backend_t io_backend;

    /* The (minimal) batch size of i/o requests being taken from a single i/o account.
    It is a factor because the actual batch size is this factor multiplied by the
    i/o priority of the account. */
    int io_batch_factor;

    /* Enable reading more data than requested to let the cache warmup more quickly esp. on rotational drives */
    bool read_ahead;

    RDB_MAKE_ME_SERIALIZABLE_8(gc_low_ratio, gc_high_ratio, num_active_data_extents, file_size, file_zone_size, io_backend, io_batch_factor, read_ahead);
};

/* This is equivalent to log_serializer_static_config_t below, but is an on-disk
structure. Changes to this change the on-disk database format! */
struct log_serializer_on_disk_static_config_t {
    uint64_t block_size_;
    uint64_t extent_size_;

    // Some helpers
    uint64_t blocks_per_extent() const { return extent_size_ / block_size_; }
    int block_index(off64_t offset) const { return (offset % extent_size_) / block_size_; }
    int extent_index(off64_t offset) const { return offset / extent_size_; }

    // Minimize calls to these.
    block_size_t block_size() const { return block_size_t::unsafe_make(block_size_); }
    uint64_t extent_size() const { return extent_size_; }
};

/* Configuration for the serializer that is set when the database is created */
struct log_serializer_static_config_t : public log_serializer_on_disk_static_config_t {
    log_serializer_static_config_t() {
        extent_size_ = DEFAULT_EXTENT_SIZE;
        block_size_ = DEFAULT_BTREE_BLOCK_SIZE;
    }

    RDB_MAKE_ME_SERIALIZABLE_2(block_size_, extent_size_);
};

#endif /* SERIALIZER_LOG_CONFIG_HPP_ */

