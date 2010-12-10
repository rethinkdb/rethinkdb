
#ifndef __SERIALIZER_LOG_EXTENT_MANAGER_HPP__
#define __SERIALIZER_LOG_EXTENT_MANAGER_HPP__

#include <sys/types.h>
#include <sys/stat.h>
#include <set>
#include <deque>
#include <unistd.h>
#include <list>
#include "utils.hpp"
#include "arch/arch.hpp"
#include "config/args.hpp"
#include "config/cmd_args.hpp"
#include "containers/segmented_vector.hpp"
#include "logger.hpp"

#define NULL_OFFSET off64_t(-1)

struct extent_zone_t;

class extent_manager_t {
public:
    struct metablock_mixin_t {
        /* When we shut down, we store the number of extents in use in the metablock.
        When we start back up and reconstruct the free list, we assert that it is the same.
        I would wrap this in #ifndef NDEBUG except that I don't want the format to change
        between debug and release mode. */
        int debug_extents_in_use;
    };

private:
    typedef std::set<off64_t> reserved_extent_set_t;
    typedef std::deque<off64_t> free_queue_t;
    
    log_serializer_static_config_t *static_config;
    log_serializer_dynamic_config_t *dynamic_config;

public:
    const uint64_t extent_size;   /* Same as static_config->extent_size */
    
public:
    class transaction_t
    {
        friend class extent_manager_t;
        transaction_t() { }
        free_queue_t free_queue;
    };

public:
    extent_manager_t(direct_file_t *file, log_serializer_static_config_t *static_config, log_serializer_dynamic_config_t *dynamic_config);
    ~extent_manager_t();

private:
    unsigned int num_zones;
    extent_zone_t *zones[MAX_FILE_ZONES];
    int next_zone;    /* Which zone to give the next extent from */
    
    extent_zone_t *zone_for_offset(off64_t offset) {
        if (dbfile->is_block_device() || dynamic_config->file_size > 0) {
            size_t zone_size = ceil_aligned(dynamic_config->file_zone_size, extent_size);
            return zones[offset / zone_size];
        } else {
            /* There is only one zone on a non-block device */
            return zones[0];
        }
    }
    
public:
    /* When we load a database, we use reserve_extent() to inform the extent manager
    which extents were already in use */
    
    void reserve_extent(off64_t extent);

public:
    void start_new();
    void start_existing(metablock_mixin_t *last_metablock);
    void prepare_metablock(metablock_mixin_t *metablock);
    void shutdown();

public:
    /* The extent manager uses transactions to make sure that extents are not freed
    before it is safe to free them. An extent manager transaction is created for every
    log serializer write transaction. Any extents that are freed in the course of
    perfoming the write transaction are recorded in a "holding area" on the extent
    manager transaction. They are only allowed to be reused after the transaction
    is committed; the log serializer only commits the transaction after the metablock
    has been written. This guarantees that we will not overwrite extents that the
    most recent metablock points to. */
    
    transaction_t *begin_transaction();
    off64_t gen_extent();
    void release_extent(off64_t extent);
    void end_transaction(transaction_t *t);
    void commit_transaction(transaction_t *t);

public:
    int held_extents();

private:
    direct_file_t *dbfile;
    
    /* During serializer startup, each component informs the extent manager
    which extents in the file it was using at shutdown. This is the
    "state_reserving_extents" phase. Then extent_manager_t::start() is called
    and it is in "state_running", where it releases and generates extents. */
    enum state_t {
        state_reserving_extents,
        state_running,
        state_shut_down
    } state;
    
    int n_extents_in_use;
    
    transaction_t *current_transaction;
};
#endif /* __SERIALIZER_LOG_EXTENT_MANAGER_HPP__ */
